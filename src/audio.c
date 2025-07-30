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

static const int TIMEOUT_DELAY_MS = 25;

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
 * @brief main loop for audio playback, a function allows for early exit.
 * @param
 * @return true on success, false on failure
 */
static bool playback_loop(const struct audio_thread_args *args) {
    //wait on mutex
    SDL_LockMutex(args->queue->mutex);

    // queue is empty
    if (args->queue->size == 0) {

        //wait
        if (!SDL_WaitConditionTimeout(args->queue->not_empty, args->queue->mutex, TIMEOUT_DELAY_MS)) {
            // It is potentially not abnormal for this to time out when ther is no audio
            SDL_UnlockMutex(args->queue->mutex);
            return true; // returns true as erroring out is not needed
        }
    }


    AVFrame *frame = dequeue_frame(args->queue);
    if (!frame) {
        SDL_UnlockMutex(args->queue->mutex);
        *args->exit_flag = false;
        return false;
    }

    SDL_Log("dequeue_frame");
    SDL_UnlockMutex(args->queue->mutex);

    if (!frame || !frame->data[0]) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "audio playback: invalid frame");
        return false;
    }



    av_frame_free(&frame);
    return true;
}

int audio_playback(void *data) {
    const struct audio_thread_args *args = (struct audio_thread_args *) data;

    SDL_ResumeAudioDevice(args->audio_device);

    while (!*args->exit_flag) {

        playback_loop(args);
        if (!playback_loop(args)) {
            *args->exit_flag = true;
        }
    }
    return 0;
}
