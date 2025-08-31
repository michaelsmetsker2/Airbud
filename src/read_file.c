/**
 * @file read_file.c
 *
 * Main file for the decoder thread.
 * Handles opening files, decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <read_file.h>
#include <decode.h>
#include <frame_queue.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>

#define SAMPLE_RATE 48000 // audio sample rate

#define VIDEO_STREAM_INDEX 1
#define AUDIO_STREAM_INDEX 3

static const char FILEPATH[] = "Z:/projects/airbud/VTS_03_0.VOB";

/**
 * @struct decoder_thread_args
 * @brief Parameters for the decoder thread.
 */
struct decoder_thread_args {
    SDL_AtomicInt *exit_flag;                  /**< 0, for continuing, 1 for soft exit decoding lool, -1 for hard exit */

    frame_queue *video_queue;                  /**< video queue to add frames to */
    SDL_AtomicU32 *total_audio_samples;        /**< total ammount of samples added to the audio queue, used to sync renderer */
    SDL_AudioStream *audio_stream;             /**< audio stream for sound playback */

    struct decoder_instructions *instructions; /**< what part of the file should be decoded, also handles swapping conds*/
};

bool create_decoder_thread(app_state *appstate) {

    SDL_SetAtomicInt(&appstate->stop_decoder_thread, 0);

    // creates playback instructions to start decoding at the main menu
    appstate->playback_instructions = malloc(sizeof(struct decoder_instructions));
    if (!appstate->playback_instructions) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate playback instructions\n");
        return false;
    }
    //populates playback instructions
    appstate->playback_instructions->start_offset_bytes = appstate->current_game_state->start_offset_bytes;
    appstate->playback_instructions->end_offset_bytes = appstate->current_game_state->end_offset_bytes;
    appstate->playback_instructions->audio_only = appstate->current_game_state->audio_only;

    appstate->playback_instructions->mutex = SDL_CreateMutex();
    if (!appstate->playback_instructions->mutex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't populate initial playback instructions\n");
        return false;
    }

    SDL_SetAtomicInt(&appstate->playback_instructions->end_reached, 0);

    // creates and populates args
    struct decoder_thread_args *args = malloc(sizeof(struct decoder_thread_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for decoder thread\n");
        return false;
    }
    args->audio_stream = appstate->audio_stream;
    args->exit_flag = &appstate->stop_decoder_thread;
    args->video_queue = appstate->render_queue;
    args->total_audio_samples = &appstate->total_audio_samples;
    args->instructions = appstate->playback_instructions;

    //starts decoder thread
    appstate->decoder_thread = SDL_CreateThread(play_file, "decoder", args);
    if (!appstate->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder thread\n");
        return false;
    }

    return true;
}

/**
 * @struct media_context
 * @brief Holds all relevant file context for video and audio decoding, makes it easier to clean up
 * one of the vobs has audio on stream 3 so just to be safe I find the correct streams for each new file
 * instead of having them as a const.
 */
struct media_context {
    AVFormatContext *format_context;         /**< information about the file being decoded */
    AVPacket        *packet;                 /**< packet of decoded data of any stream */

    AVCodecContext  *video_codec_ctx;        /**< decodec for decoding the video stream */
    AVFrame         *video_frame;            /**< reused video frame, its data is copied to a queue */

    AVCodecContext  *audio_codec_ctx;        /**< decodec for decoding the audio stream */
    AVFrame         *audio_frame;            /**< reused audio frame, its data is copied to a queue */
    SwrContext      *resample_context;       /**< software resampler to make audio usable in SDL3 */
};

/**
 * @brief cleanly destroys a media_context struct
 * @param ctx struct to destroy
 */
static void destroy_media_context(struct media_context *ctx) {
    av_frame_free(&ctx->video_frame);
    av_frame_free(&ctx->audio_frame);
    av_packet_free(&ctx->packet);
    swr_free(&ctx->resample_context);
    avcodec_free_context(&ctx->video_codec_ctx);
    avcodec_free_context(&ctx->audio_codec_ctx);
    avformat_close_input(&ctx->format_context);
}

/**
 * Initializes the media context by opening the input file and preparing
 * the codec, format context, and frame/packet allocations for video decoding
 * //TODO hardcode decodecs
 *
 * @param media_ctx Pointer to the media context to initialize
 * @return true on success, false on failure. On failure, no cleanup is performed.
 */
static bool setup_file_context(struct media_context *media_ctx) {

    // opens the file (only looks at header)
    if (avformat_open_input(&media_ctx->format_context, FILEPATH, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        return false;
    }
    // finds the streams info
    if (avformat_find_stream_info(media_ctx->format_context, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        return false;
    }

    const AVCodecParameters *video_codec_par = media_ctx->format_context->streams[VIDEO_STREAM_INDEX]->codecpar;
    const AVCodecParameters *audio_codec_par = media_ctx->format_context->streams[AUDIO_STREAM_INDEX]->codecpar;

    //finds correct decodecs
    const AVCodec *video_codec = avcodec_find_decoder(video_codec_par->codec_id);
    const AVCodec *audio_codec = avcodec_find_decoder(audio_codec_par->codec_id);
    if (!video_codec || !audio_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find decoder\n");
        return false;
    }

    // Allocates memory for codec context
    media_ctx->video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!media_ctx->video_codec_ctx) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return false;
    }
    media_ctx->audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (!media_ctx->audio_codec_ctx) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio codec context\n");
        return false;
    }

    // populates codec information
    if (avcodec_parameters_to_context(media_ctx->video_codec_ctx, video_codec_par) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
        return false;
    }
    if (avcodec_parameters_to_context(media_ctx->audio_codec_ctx, audio_codec_par) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy audio codec parameters\n");
        return false;
    }

    // Opens decoders
    if (avcodec_open2(media_ctx->video_codec_ctx, video_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open codec\n");
        return false;
    }
    if (avcodec_open2(media_ctx->audio_codec_ctx, audio_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open audio codec\n");
        return false;
    }

    // Initializes resampler
    if (swr_alloc_set_opts2(&media_ctx->resample_context,
        &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO,
        AV_SAMPLE_FMT_S16,
        SAMPLE_RATE,

        &media_ctx->audio_codec_ctx->ch_layout,
        media_ctx->audio_codec_ctx->sample_fmt,
        media_ctx->audio_codec_ctx->sample_rate,
        0, NULL
        ) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate resampler\n");
        return false;
    }

    // Initialize resampler
    if (swr_init(media_ctx->resample_context) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't initialize resampler\n");
        swr_free(&media_ctx->resample_context);
        return false;
    }

    //alloc packet and video_frame
    media_ctx->packet = av_packet_alloc();
    media_ctx->video_frame = av_frame_alloc();
    media_ctx->audio_frame = av_frame_alloc();
    if (!media_ctx->packet || !media_ctx->video_frame || !media_ctx->audio_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate packet or frame\n");
        return false;
    }

    return true;
}

/**
 * @brief main decoding loop, segmented for easy early break
 * breaks on exit_flag 1 or -1 (for hard exit)
 *
 * TODO check if end of file?
 *
 * @param args thread args passed through
 * @param media_ctx file and decodec information
 * @param current_offset_bytes current position of the packet in the file
 * @return true on clean exit, false otherwise
 */
static bool decode_loop(const struct decoder_thread_args *args, const struct media_context *media_ctx, uint64_t *current_offset_bytes)  {

    // while there is unparsed data left in the file
    while (!SDL_GetAtomicInt(args->exit_flag) && av_read_frame(media_ctx->format_context, media_ctx->packet) >= 0) {

        // increment the current offset
        *current_offset_bytes += media_ctx->packet->size;

        if (*current_offset_bytes > args->instructions->end_offset_bytes) {
            //the end of the section to decode has been reached

            SDL_Log("end of section decoded \n");



            // waits for audio queue to empty
            while (SDL_GetAudioStreamAvailable(args->audio_stream) > 0 ) {
                if (SDL_GetAtomicInt(args->exit_flag) != 0) {
                    return true;
                }
                SDL_Log("waitin on audio to flush \n");
                SDL_Delay(1);
            }

            SDL_Log("audio is empty \n");

            //waits for video queue to empty (if there is one more frames left after audio)
            SDL_LockMutex(args->video_queue->mutex);
            SDL_WaitCondition(args->video_queue->empty, args->video_queue->mutex);
            SDL_UnlockMutex(args->video_queue->mutex);

            SDL_Log("video is empty \n");

            //Tells main thread
            SDL_SetAtomicInt(&args->instructions->end_reached, 1);

            SDL_SetAtomicInt(args->exit_flag, 1);
            return true;
        }

        if (media_ctx->packet->stream_index == AUDIO_STREAM_INDEX) {
            // if packet is in the audio stream, decode it

            if (!decode_audio(media_ctx->audio_codec_ctx, media_ctx->packet, media_ctx->audio_frame, media_ctx->resample_context,
                args->audio_stream, args->total_audio_samples))
            {
                return false;
            }

        } else if (media_ctx->packet->stream_index == VIDEO_STREAM_INDEX) {
            // packet is in video stream, decode it

            if (args->instructions->audio_only) {
                av_packet_unref(media_ctx->packet);
            } else {
                if (!decode_video(media_ctx->video_codec_ctx, media_ctx->packet, media_ctx->video_frame, args->video_queue)) {
                    return false;
                }
            }
        }
        av_packet_unref(media_ctx->packet);
    }
    return true;
}

int play_file(void *data) {
    const struct decoder_thread_args *args = data;

    // Sets up media context struct
    struct media_context media_ctx = {0};
    if (!setup_file_context(&media_ctx)) {
        destroy_media_context(&media_ctx);
        //FIXME free args and cleanup?
        return -1;
    }

    //plays a section of the file specified by instructions, setting exit flag to one exits out
    while (SDL_GetAtomicInt(args->exit_flag) != -1 ) {

        //TODO check validity of instructions

        //resets exit flag
        SDL_SetAtomicInt(args->exit_flag, 0);
        //keeps main thread from changing gamestate while decoding
        SDL_LockMutex(args->instructions->mutex);

        //seeks to start of instructed sequence of bytes
        if (av_seek_frame(media_ctx.format_context, -1, args->instructions->start_offset_bytes, AVSEEK_FLAG_BYTE) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't seek to the given byte offset\n");
            break;
        }
        uint64_t current_offset_bytes = args->instructions->start_offset_bytes;

        avcodec_flush_buffers(media_ctx.audio_codec_ctx);
        avcodec_flush_buffers(media_ctx.video_codec_ctx);

        if (!decode_loop(args, &media_ctx, &current_offset_bytes)) {
            break;
        }

        // this lifts the mutex lock so the main thread can change the values;
        SDL_UnlockMutex(args->instructions->mutex);
        SDL_Delay(4); // gives time for main thread to grab mutex
    }

    // there is no new instructions or there was an error
    // either way clean up
    // TODO is decoder args getting cleaned up?

    destroy_media_context(&media_ctx);
    return 0;
}