/**
 * @file playback.c
 *
 * Handles decoding and adding frames to the queue.
 *
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <SDL3/SDL.h>

#include <playback.h>
#include <decode.h>

#include <libswresample/swresample.h>

#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>


/**
 * @struct media_context
 * @brief Holds all relevant file context for video and audio decoding, makes it easier to clean up
 * one of the vobs has audio on stream 3 so just to be safe I find the correct streams for each new file
 * instead of having them as a const.
 */
struct media_context {
    AVFormatContext *format_context;

    AVCodecContext  *video_codec_ctx;        /**< decodec for decoding the video stream */
    int             video_stream_index;      /**< index of the video stream to be decoded */
    AVFrame         *video_frame;            /**< reused videoframe, its data is copied to a queue */

    // Audio
    SwrContext      *resample_context;      /**< software resampler to make audio usable in SDL3 */
    AVCodecContext  *audio_codec_ctx;       /**< decodec for decoding the audio stream */
    int             audio_stream_index;     /**< index of the audio stream to be decoded */
    AVFrame         *audio_frame;       //todo

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

    /* Finds best video codec and stream */ //TODO p sure all these are the same
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best audio stream or decoder \n"); //TODO potentially not fatal?
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
        &(AVChannelLayout){
            .order = AV_CHANNEL_ORDER_NATIVE,
            .nb_channels = 2                 // Output: stereo
        },
        AV_SAMPLE_FMT_S16,
        4800,

        &media_ctx->audio_codec_ctx->ch_layout,
        media_ctx->audio_codec_ctx->sample_fmt,
        media_ctx->audio_codec_ctx->sample_rate,
        0, NULL
        ) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't initizalize resampler\n");
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
    const struct playback_args *args = (struct playback_args *) data;
    *args->exit_flag = false; //TODO setting it here, could change, remember

    // Sets all members to NULL
    struct media_context media_ctx = {0};

    /* set up file context */
    if (!setup_file_context(&media_ctx, args->filename)) {
        destroy_media_context(&media_ctx);
        return -1;
    }

    //while there is unparsed data left in the file
    while (!*args->exit_flag && av_read_frame(media_ctx.format_context, media_ctx.packet) >= 0) {

        //does packet belong to the video stream TODO make sure this check is needed
        if (!*args->exit_flag && media_ctx.packet->stream_index == media_ctx.audio_stream_index) {

            decode_audio(media_ctx.audio_codec_ctx, media_ctx.packet, media_ctx.audio_frame, media_ctx.resample_context,
                args->queue, args->exit_flag);

        } else if (!*args->exit_flag && media_ctx.packet->stream_index == media_ctx.video_stream_index) {

            // TODO make this return a value so it can error out, can change if dropping frames tends to happen
            decode_video(media_ctx.video_codec_ctx, media_ctx.packet, media_ctx.video_frame, args->queue, args->exit_flag);
        }

        av_packet_unref(media_ctx.packet);
    }

    destroy_media_context(&media_ctx);
    return 0;
}