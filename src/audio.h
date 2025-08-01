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

#include <SDL3/SDL.h>

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
    SDL_AtomicInt *exit_flag;           /**< default 0, 1 whem the thread should stop executing */

    frame_queue *queue;                 /**< frame queue to pull frames from */
    SDL_AudioStream  *stream;           /**< audio stream for playback */

    SDL_AtomicU32 *playback_time;       /**< amount of packets of audio played, used to sync video */
};

/**
 * @brief resamples vob audio to be playable by SDL3
 *
 * @param input_frame audio frame to be resampled
 * @param output_frame resampled frame
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
