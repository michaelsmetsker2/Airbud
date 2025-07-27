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

#include <stdbool.h>
#include <decode.h>
#include <resample.h>

#include <frame-queue.h>

//TODO the majority of this file is duplicate code, room to improve

static const Sint32 TIMEOUT_DELAY_MS = 125;

void decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler,
                  frame_queue *queue, volatile bool *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        av_frame_unref(frame);
        *exit_flag = true;
    }

    //a full frame is ready
    while (!*exit_flag && avcodec_receive_frame(dec_ctx, frame) == 0) {
        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == AUDIO_BUFFER_CAP) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for queue to empty timed out\n");
                av_frame_unref(frame);
                //*exit_flag = true;
                break;
            }
        }

        if (!resample(&frame, resampler)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to resample audio frame\n");
            *exit_flag = true;
            SDL_UnlockMutex(queue->mutex);
            av_frame_unref(frame);
            break;
        }

        if (!enqueue_frame(queue, frame)) {
            *exit_flag = true;
            SDL_UnlockMutex(queue->mutex);
            av_frame_unref(frame);
            break;
        }
        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
}

void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet,
                  AVFrame *frame, frame_queue *queue,
                  volatile bool *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        av_frame_unref(frame);
        *exit_flag = true;
    }

    //a full frame is ready
    while (!*exit_flag && avcodec_receive_frame(dec_ctx, frame) == 0) {

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == VIDEO_BUFFER_CAP) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for queue to empty timed out\n");
                av_frame_unref(frame);
                *exit_flag = true;
                break;
            }
        }

        //TODO create copy locally for genericization
        if (!enqueue_frame(queue, frame)) {
            *exit_flag = true;
            SDL_UnlockMutex(queue->mutex);
            av_frame_unref(frame);
            break;
        }

        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
}
