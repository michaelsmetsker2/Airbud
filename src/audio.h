/**
 * @file audio.h
 *
 * Handles the audio thread, playback and resampling
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <frame_queue.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <stdbool.h>

/**
 * @struct audio_thread_args
 * @brief Parameters for the audio thread.
 *
 * This struct is less complex than decoder_thread_args and just a subset of appstate\
 * it does not warrent a constructor function
 */
struct audio_thread_args {
    volatile bool *exit_flag;           /**< default false, whether the thread should stop executing */

    frame_queue *queue;                 /**< frame queue to pull frames from */
    SDL_AudioDeviceID  audio_device;    /**< ID of the audio device that will play back sound */
};

/**
 * @brief resamples vob audio to be playable by SDL3
 *
 * @param input_frame audio frame to be resampled
 * @param output_frame
 * @param resampler software resample context to use
 * @return true on success false otherwise
 */
bool resample(const AVFrame *input_frame, AVFrame *output_frame, SwrContext *resampler);

/**
 * @brief Audio playback thread that pulls audio frames from the audio queue
 *
 * @param data pointer to an audio_thread_args struct with all necessary info
 * @return 0 on success negative number on error
 */
int audio_playback(void *data);

#endif //AUDIO_H
