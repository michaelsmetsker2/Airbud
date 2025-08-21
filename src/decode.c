/**
 * @file decode.c
 * Decodes video and audio packets
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libswresample/swresample.h>

#include <decode.h>
#include <frame_queue.h>

static const Sint32 TIMEOUT_DELAY_MS = 400;

static const int SAMPLE_RATE = 48000;

bool decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler,
                  SDL_AudioStream *stream, SDL_AtomicU32 *total_audio_samples)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        return false;
    }

    //a full frame is ready, can be more than one, doesn't ever seem to happen
    while (avcodec_receive_frame(dec_ctx, frame) == 0) {

        //create resampled frame
        AVFrame *frame_resampled = av_frame_alloc();
        if (!frame_resampled) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't alloc resampled frame");
            return false;
        }
        //fill new frames values
        av_channel_layout_default(&frame_resampled->ch_layout, 2);
        frame_resampled->format = AV_SAMPLE_FMT_S16;
        frame_resampled->sample_rate = SAMPLE_RATE;
        frame_resampled->nb_samples = frame->nb_samples;
        if (av_frame_get_buffer(frame_resampled, 0) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy buffer");
            av_frame_free(&frame_resampled);
            return false;
        }

        // convert the frame
        if (swr_convert_frame(resampler, frame_resampled, frame) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't resample audio frame %s", SDL_GetError());
            av_frame_free(&frame_resampled);
            return false;
        }
        // add data to queue
        const int data_size = av_samples_get_buffer_size(NULL, 2, frame_resampled->nb_samples, AV_SAMPLE_FMT_S16, 0);
        if (!SDL_PutAudioStreamData(stream, frame_resampled->data[0], data_size)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't push frame data to audio stream %s", SDL_GetError());
            return false;
        }

        // increment total samples
        uint32_t prev_samples;
        do {
            prev_samples = SDL_GetAtomicU32(total_audio_samples);
        } while (!SDL_CompareAndSwapAtomicU32(total_audio_samples, prev_samples, prev_samples + frame->nb_samples));

        SDL_SetAtomicU32(total_audio_samples, prev_samples + frame->nb_samples);

        av_frame_unref(frame);
        av_frame_free(&frame_resampled);
    }
    return true;
}

bool decode_video(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, frame_queue *queue)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        return false;
    }

    //a full frame is ready, can be more than one, doesn't ever seem to happen
    while ( avcodec_receive_frame(dec_ctx, frame) == 0) {

        SDL_LockMutex(queue->mutex); //waits for mutex

        //queue is at capacity
        if (queue->size == queue->capacity) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for video queue to empty timed out\n");
                return false;
            }
        }

        if (!enqueue_frame(queue, frame)) {
            SDL_UnlockMutex(queue->mutex);
            return false;
        }

        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
    return true;
}