/**
 * @file decode.c
 * Decodes video packets //todo update this to include audio depending on my decisions
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>

#include <stdbool.h>
#include <decode.h>

#include <frame-queue.h>

static const Sint32 TIMEOUT_DELAY_MS = 125;

void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet,
                  AVFrame *frame, frame_queue *queue,
                  volatile bool *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
    }

    //a full frame is ready
    while (!*exit_flag && avcodec_receive_frame(dec_ctx, frame) == 0) {

        //clone the frame
        AVFrame *frame_copy = av_frame_clone(frame);
        if (!frame_copy) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame\n");
            *exit_flag = true;
            break;
        }

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        if (queue->size != VIDEO_BUFFER_CAP) {
            //queue is not at capacity

            if (!enqueue_frame(queue, frame_copy)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                av_frame_free(&frame_copy);
                *exit_flag = true;
                SDL_UnlockMutex(queue->mutex);
                break;
            }
        }
        else {
            //queue is at capacity
            if (SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                if (!enqueue_frame(queue, frame_copy)) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue timed out\n");
                    av_frame_free(&frame_copy);
                    *exit_flag = true;
                    SDL_UnlockMutex(queue->mutex);
                    break;
                }
            }
        }
        SDL_UnlockMutex(queue->mutex);
    }
}