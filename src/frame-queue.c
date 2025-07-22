/**
 * @file frame-queue.c
 *
 * helper functions for FrameQueues
 *
 * @author Michael Metsker
 * @version 1.0
*/

#include "../include/frame-queue.h"

void destroyFrame(Frame *frame) {
    if (!frame) {
        return;
    }

    if (frame->audioFrame) {
        av_frame_free(&frame->audioFrame);
    }
    if (frame->videoFrame) {
        av_frame_free(&frame->videoFrame);
    }

    //TODO remove menu button data as well
}

FrameQueue *createFrameQueue() {
    FrameQueue *queue = malloc(sizeof(FrameQueue));
    if (!queue) return NULL;

    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;

    queue->mutex = SDL_CreateMutex();
    queue->notEmpty = SDL_CreateCondition();
    queue->notFull = SDL_CreateCondition();

    if (!queue->mutex || !queue->notEmpty || !queue->notFull) {
        SDL_DestroyMutex(queue->mutex);
        SDL_DestroyCondition(queue->notEmpty);
        SDL_DestroyCondition(queue->notFull);
        free(queue);
        return NULL;
    }

    return queue;
}

bool enqueueFrame(FrameQueue *queue, Frame *frame) {

    //queue full, should not even be called if this is the case
    while (queue->size == FRAME_QUEUE_CAPACITY) {
        SDL_UnlockMutex(queue->mutex);
        return false;
    }

    queue->frames[queue->rear] = frame;
    queue->rear = (queue->rear + 1) % FRAME_QUEUE_CAPACITY;
    queue->size++;

    SDL_SignalCondition(queue->notEmpty);
    return true;
}

Frame *dequeueFrame(FrameQueue *queue) {

    // if queue is empty, shouldn't even be called if this is the case
    if (queue->size == 0) {
        SDL_UnlockMutex(queue->mutex);
        return NULL;
    }

    Frame *frame = queue->frames[queue->front];
    queue->front = (queue->front + 1) % FRAME_QUEUE_CAPACITY;
    queue->size--;


    SDL_SignalCondition(queue->notFull);
    return frame;
}

void destroyFrameQueue(FrameQueue *queue) {
    if (!queue) return;

    SDL_LockMutex(queue->mutex); //TODO idk if this is needed, time will come

    // free all frames in the queue
    for (int i = 0; i < FRAME_QUEUE_CAPACITY; i++) {
        if (queue->frames[i]) {
            destroyFrame(queue->frames[i]);
        }
    }

    SDL_UnlockMutex(queue->mutex); //TODO here as well
    SDL_DestroyMutex(queue->mutex);

    SDL_DestroyCondition(queue->notEmpty);
    SDL_DestroyCondition(queue->notFull);
    free(queue);
}
