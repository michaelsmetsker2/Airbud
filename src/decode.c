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

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == VIDEO_BUFFER_CAP) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for queue to empty timed out\n");
                //*exit_flag = true; //TODO if i dont wanna drop frames, uncomment
                break;
            }
        }

        if (!enqueue_frame(queue, frame)) {
            *exit_flag = true;
            SDL_UnlockMutex(queue->mutex);
            break;
        }

        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
}