/**
 * @file resample.h
 * resamples audio frames so they can be used with SDL3
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef RESAMPLE_H
#define RESAMPLE_H

#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <stdbool.h>

/**
 * @brief resamples vob audio to be playable by SDL3
 *
 * @param frame audio frame to be resampled
 * @param resampler software resample context to use
 * @return true on success false otherwise
 */
bool resample(AVFrame **frame, SwrContext *resampler);

#endif //RESAMPLE_H
