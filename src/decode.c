/**
 * @file decode.c
 * Decodes video packets //todo update this to include audio depending on my decisions
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include "decode.h"

#include <libavcodec/avcodec.h>

static const Sint32 TIMEOUT_DELAY_MS = 125; //TODO prolly define in common

void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet,
                  AVFrame *frame, frame_queue *queue,
                  volatile bool *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
    }

    //a full frame is ready
    while (!(*exit_flag) && avcodec_receive_frame(dec_ctx, frame) == 0) {

        //clone the frame
        AVFrame *frame_copy = av_frame_clone(frame);
        if (!frame_copy) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame\n");
            *exit_flag = true;
            break;
        }

        //wrap decoded video frame in frame struct
        struct frame *frame_wrapper = malloc(sizeof(struct frame));
        if (!frame_wrapper) {
            av_frame_free(&frame_copy);
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frameWrapper\n");
            *exit_flag = true;
            break;
        }

        frame_wrapper->video_frame = frame_copy;
        // TODO this is where game_state might be assigned idk yet and buttons

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        if (queue->size != FRAME_QUEUE_CAPACITY) {
            //queue is not at capacity

            if (!enqueue_frame(queue, frame_wrapper)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                destroy_frame(frame_wrapper);
                *exit_flag = true;
                SDL_UnlockMutex(queue->mutex);
                break;
            }
        } else {
            //queue is at capacity
            if (SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                if (!enqueue_frame(queue, frame_wrapper)) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue timed out\n");
                    destroy_frame(frame_wrapper);
                    *exit_flag = true;
                    SDL_UnlockMutex(queue->mutex);
                    break;
                }
            }
        }
        SDL_UnlockMutex(queue->mutex);
    }

}

