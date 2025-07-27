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
 *
 *
 *
 */
bool resample(AVFrame *frame, SwrContext resampler);





#endif //RESAMPLE_H
