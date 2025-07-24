//
// Created by micha on 7/23/2025.
//

#include "decode.h"


void decode_video(AVPacket *packet) {

    //decodes packet
    if (avcodec_send_packet(video_codec_context, packet) == 0) {
        //a full frame is ready (in case multiple frames in a single packed)
        while (!(*args->exit_flag) && avcodec_receive_frame(video_codec_context, video_frame) == 0) {
            if decode_video_frame



            AVFrame *cloned_video_frame = av_frame_clone(video_frame); //clone the frame
            if (!cloned_video_frame) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame\n");
                break; //can't just return as cleanup is needed
            }

            //wrap decoded video frame in frame struct
            struct frame *frameWrapper = malloc(sizeof(struct frame));
            if (!frameWrapper) {
                av_frame_free(&cloned_video_frame);
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frameWrapper\n");
                break;
            }

            frameWrapper->video_frame = cloned_video_frame;
            frameWrapper->audio_frame = NULL; //TODO audio implementation
            //TODO button implementation

            //TODO should definitely make this while loop a function
            //loop mutex is available and the queue is not full
            while (!(*args->exit_flag)) {
                SDL_LockMutex(args->queue->mutex); // waits until mutex is unlocked

                if (!(*args->exit_flag) && args->queue->size != FRAME_QUEUE_CAPACITY) {
                    //queue is not at capacity
                    if (!enqueue_frame(args->queue, frameWrapper)) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                        destroy_frame(frameWrapper);
                        break;
                    }
                } else {
                    //queue is at capacity
                    if (SDL_WaitConditionTimeout(args->queue->not_full, args->queue->mutex, TIMEOUT_DELAY_MS)) {
                        if (!enqueue_frame(args->queue, frameWrapper)) {
                            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                            destroy_frame(frameWrapper);
                            break;
                        }
                    } // timed out
                }
                SDL_UnlockMutex(args->queue->mutex);
            }
        }
    }

}

