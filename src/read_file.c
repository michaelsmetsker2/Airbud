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

/**
 * @struct decoder_thread_args
 * @brief Parameters for the decoder thread.
 */
struct decoder_thread_args {
    SDL_AtomicInt *exit_flag;           /**< 0, 1 whether the thread should stop executing */

    frame_queue *video_queue;           /**< video queue to add frames to */
    SDL_AudioStream *audio_stream;      /**< audio stream for sound playback */
    SDL_AtomicU32 *total_audio_samples; /**< total ammount of samples added to the audio queue, used to sync renderer */

    const char *filename;               /**< file to be played back */
};

bool create_decoder_thread(app_state *appstate, const char *filename) {

    // creates and populates args
    struct decoder_thread_args *args = malloc(sizeof(struct decoder_thread_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for decoder thread\n");
        return false;
    }
    args->audio_stream = appstate->audio_stream;
    args->exit_flag = &appstate->stop_decoder_thread;
    args->video_queue = appstate->render_queue;
    args->filename = filename;
    args->total_audio_samples = &appstate->total_audio_samples;

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
    AVFormatContext *format_context;

    AVCodecContext  *video_codec_ctx;        /**< decodec for decoding the video stream */
    int              video_stream_index;     /**< index of the video stream to be decoded */
    AVFrame         *video_frame;            /**< reused video frame, its data is copied to a queue */

    // Audio
    SwrContext      *resample_context;      /**< software resampler to make audio usable in SDL3 */
    AVCodecContext  *audio_codec_ctx;       /**< decodec for decoding the audio stream */
    int              audio_stream_index;    /**< index of the audio stream to be decoded */
    AVFrame         *audio_frame;           /**< reused audio frame, its data is copied to a queue */

    AVPacket        *packet;                /**< packet of decoded data of any stream */
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
 *
 * TODO turn a bunch of this into preset values to speed up initialization
 *
 * @param media_ctx Pointer to the media context to initialize
 * @param filename  Path to the media file to open
 * @return true on success, false on failure. On failure, no cleanup is performed.
 */
static bool setup_file_context(struct media_context *media_ctx, const char *filename) {

    // opens the file (only looks at header)
    if (avformat_open_input(&media_ctx->format_context, filename, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        return false;
    }
    // finds the streams info
    if (avformat_find_stream_info(media_ctx->format_context, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        return false;
    }

    /* Finds best video codec and stream */
    const AVCodec *video_codec = NULL;
    media_ctx->video_stream_index = av_find_best_stream(media_ctx->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (media_ctx->video_stream_index < 0 || !video_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best video stream or decoder \n");
        return false;
    }
    /* Finds best audio codec and stream */
    const AVCodec *audio_codec = NULL;
    media_ctx->audio_stream_index = av_find_best_stream(media_ctx->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    if (media_ctx->audio_stream_index < 0 || !video_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best audio stream or decoder \n");
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

    { //variable declarations are just for readability
        const AVCodecParameters *video_codec_parameters = media_ctx->format_context->streams[media_ctx->video_stream_index]->codecpar;
        if (avcodec_parameters_to_context(media_ctx->video_codec_ctx, video_codec_parameters) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
            return false;
        }
        const AVCodecParameters *audio_codec_parameters = media_ctx->format_context->streams[media_ctx->audio_stream_index]->codecpar;
        if (avcodec_parameters_to_context(media_ctx->audio_codec_ctx, audio_codec_parameters) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy audio codec parameters\n");
            return false;
        }
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
        48000,

        &media_ctx->audio_codec_ctx->ch_layout,
        media_ctx->audio_codec_ctx->sample_fmt,
        media_ctx->audio_codec_ctx->sample_rate,
        0, NULL
        ) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't initizalize resampler\n");
        return false;
    }

    // Initialize resampler
    if (swr_init(media_ctx->resample_context) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't initialize resampler (swr_init failed)\n");
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

int play_file(void *data) {
    const struct decoder_thread_args *args = (struct decoder_thread_args *) data;

    // Sets all members to NULL
    struct media_context media_ctx = {0};

    /* set up file context */
    if (!setup_file_context(&media_ctx, args->filename)) {
        destroy_media_context(&media_ctx);
        return -1;
    }

    //while there is unparsed data left in the file
    while (!SDL_GetAtomicInt(args->exit_flag) && av_read_frame(media_ctx.format_context, media_ctx.packet) >= 0) {

        if (media_ctx.packet->stream_index == media_ctx.audio_stream_index) {

            // TODO prolly should make these return a value on failure
            decode_audio(media_ctx.audio_codec_ctx, media_ctx.packet, media_ctx.audio_frame, media_ctx.resample_context,
                args->exit_flag, args->audio_stream, args->total_audio_samples);

        } else if (media_ctx.packet->stream_index == media_ctx.video_stream_index) {

            decode_video(media_ctx.video_codec_ctx, media_ctx.packet, media_ctx.video_frame, args->video_queue, args->exit_flag);
        }

        av_packet_unref(media_ctx.packet);
    }

    destroy_media_context(&media_ctx);
    //TODO is decoder args getting cleaned up?

    return 0;
}