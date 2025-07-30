/**
 * @file audio.h
 *
 * Handles the audio thread, playback and resampling
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <audio.h>

bool resample(const AVFrame *input_frame, AVFrame *output_frame, SwrContext *resampler) {


    output_frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&output_frame->ch_layout, 2);
    output_frame->sample_rate = 48000;
    output_frame->nb_samples = input_frame->nb_samples;

    if (av_frame_get_buffer(output_frame, 0) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy buffer");
        av_frame_free(&output_frame);
        return false;
    }

    if (swr_convert_frame(resampler, output_frame, input_frame) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't resample audio frame");
        av_frame_free(&output_frame);
        return false;
    }

    return true;
}
/*
struct decoder_thread_args *create_decoder__args(const struct app_state *appstate, const char *filename) {

    struct decoder_thread_args *args = malloc(sizeof(struct decoder_thread_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for decoder thread\n");
        return NULL;
    }

    args->exit_flag = appstate->stop_decoder_thread;
    args->video_queue = appstate->video_queue;
    args->audio_queue = appstate->audio_queue;
    args->filename = filename;

    return args;
}

int play_file(void *data) {
    const struct decoder_thread_args *args = (struct decoder_thread_args *) data;
    *args->exit_flag = false; //TODO setting it here, could change, remember

    // Sets all members to NULL
    struct media_context media_ctx = {0};

    if (!setup_file_context(&media_ctx, args->filename)) {
        destroy_media_context(&media_ctx);
        return -1;
    }

    return 0;
}
 */
