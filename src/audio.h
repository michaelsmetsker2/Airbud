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

#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <stdbool.h>

/**
 * @brief resamples vob audio to be playable by SDL3
 *
 * @param input_frame audio frame to be resampled
 * @param output_frame
 * @param resampler software resample context to use
 * @return true on success false otherwise
 */
bool resample(const AVFrame *input_frame, AVFrame *output_frame, SwrContext *resampler);

#endif //AUDIO_H
