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

static const int SAMPLE_RATE = 48000;

static const int TIMEOUT_DELAY_MS = 25;

bool resample(const AVFrame *input_frame, AVFrame *output_frame, SwrContext *resampler) {
    //fill output frame parameters
    av_channel_layout_default(&output_frame->ch_layout, 2);
    output_frame->format = AV_SAMPLE_FMT_S16;
    output_frame->sample_rate = SAMPLE_RATE;
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
        //wait untill not empty
        if (!SDL_WaitConditionTimeout(args->queue->not_empty, args->queue->mutex, TIMEOUT_DELAY_MS)) {
            // It is not abnormal for this to time out when ther is no audio
            SDL_UnlockMutex(args->queue->mutex);
            return true;
        }
    }
    //dequeues frame
    AVFrame *frame = dequeue_frame(args->queue);
    if (!frame) {
        SDL_UnlockMutex(args->queue->mutex);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't get dequeued audio frame");
        SDL_SetAtomicInt(args->exit_flag, 1);
        return false;
    }

    // add data to queue
    const int data_size = av_samples_get_buffer_size(NULL, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (!SDL_PutAudioStreamData(args->stream, frame->data[0], data_size)) {
        SDL_UnlockMutex(args->queue->mutex);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't push frame data to audio stream");
        SDL_SetAtomicInt(args->exit_flag, 1);
        return false;
    }

    // increment playback_time
    const uint32_t old_val = SDL_GetAtomicU32(args->playback_time);
    const uint32_t new_val = ( frame->nb_samples * 1000) / SAMPLE_RATE + old_val;
    SDL_SetAtomicU32(args->playback_time, new_val);


    SDL_UnlockMutex(args->queue->mutex);
    av_frame_free(&frame);
    return true;
}

int audio_playback(void *data) {
    const struct audio_thread_args *args = (struct audio_thread_args *) data;

    SDL_ResumeAudioStreamDevice(args->stream);

    while (!SDL_SetAtomicInt(args->exit_flag, 0)) {

        playback_loop(args);
        if (!playback_loop(args)) {
            !SDL_SetAtomicInt(args->exit_flag, 0);
        }
    }

    //TODO no way to differentiate error or standard exit flag

    SDL_PauseAudioStreamDevice(args->stream);
    if (!SDL_FlushAudioStream(args->stream)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't flush audio stream");
    }
    //TODO close stream

    return 0;
}
