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
#include <audio.h>

#include <frame_queue.h>

//TODO the majority of this file is duplicate code, room to improve

static const Sint32 TIMEOUT_DELAY_MS = 450;

void decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler,
                  frame_queue *queue, SDL_AtomicInt *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        //SDL_SetAtomicInt(exit_flag, 1); TODO some files start half way through packet??
    }

    //a full frame is ready
    while (!SDL_GetAtomicInt(exit_flag) && avcodec_receive_frame(dec_ctx, frame) == 0) {

        //resample frame before holding mutex
        AVFrame *frame_resampled = av_frame_alloc();
        if (!frame_resampled) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy");
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }

        if (!resample(frame, frame_resampled, resampler)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to resample audio frame\n");
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }
        av_frame_unref(frame);

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == AUDIO_BUFFER_CAP) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for audio queue to empty timed out\n");
                av_frame_free(&frame_resampled);
                break;
            }
        }

        if (!enqueue_frame(queue, frame_resampled)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue audio frame\n");
            av_frame_free(&frame_resampled);
            SDL_SetAtomicInt(exit_flag, 1);
            SDL_UnlockMutex(queue->mutex);
            break;
        }

        SDL_UnlockMutex(queue->mutex);
    }
}

void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet,
                  AVFrame *frame, frame_queue *queue,
                  SDL_AtomicInt *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        SDL_SetAtomicInt(exit_flag, 1);
    }

    //a full frame is ready
    while (!SDL_GetAtomicInt(exit_flag) && avcodec_receive_frame(dec_ctx, frame) == 0) {

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == VIDEO_BUFFER_CAP) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for video queue to empty timed out\n");
                SDL_SetAtomicInt(exit_flag, 1);
                break;
            }
        }

        //TODO copying frame while holding mutex will slow down the queue
        //clone the frame
        AVFrame *frame_copy = av_frame_clone(frame);
        if (!frame_copy) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame for queueing\n");
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }

        if (!enqueue_frame(queue, frame_copy)) {
            av_frame_free(&frame_copy);
            SDL_SetAtomicInt(exit_flag, 1);
            SDL_UnlockMutex(queue->mutex);
            break;
        }

        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
}
