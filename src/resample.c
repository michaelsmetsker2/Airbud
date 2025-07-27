/**
* @file resample.h
 * resamples audio frames so they can be used with SDL3
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <resample.h>

bool resample(AVFrame **frame, SwrContext *resampler) {

    //TODO technically this is makeing a second clone before queuing sot here is room to optimise
    AVFrame *output_frame = av_frame_alloc();
    if (!output_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy");
        return false;
    }

    output_frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&output_frame->ch_layout, 2);
    output_frame->sample_rate = 48000;
    output_frame->nb_samples = (*frame)->nb_samples;

    if (av_frame_get_buffer(output_frame, 0) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy buffer");
        av_frame_free(&output_frame);
        return false;
    }

    if (swr_convert_frame(resampler, output_frame, *frame) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't resample audio frame");
        av_frame_free(&output_frame);
        return false;
    }

    /*
    AVFrame *old_frame = *frame;
    *frame = output_frame;
    av_frame_free(&old_frame);  // unref + free internally
*/
    return true;
}