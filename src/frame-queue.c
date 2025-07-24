/**
 * @file frame-queue.c
 *
 * helper functions for frame and frame_queue structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include "../include/frame-queue.h"

void destroy_frame(struct frame *frame) {
    if (!frame) {
        return;
    }

    if (frame->audio_frame) {
        av_frame_free(&frame->audio_frame);
    }
    if (frame->video_frame) {
        av_frame_free(&frame->video_frame);
    }
    //TODO remove menu button data as well
}

frame_queue *create_frame_queue() {
    frame_queue *queue = malloc(sizeof(frame_queue));
    if (!queue) return NULL;

    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;

    queue->mutex = SDL_CreateMutex();
    queue->not_empty = SDL_CreateCondition();
    queue->not_full = SDL_CreateCondition();

    if (!queue->mutex || !queue->not_empty || !queue->not_full) {
        SDL_DestroyMutex(queue->mutex);
        SDL_DestroyCondition(queue->not_empty);
        SDL_DestroyCondition(queue->not_full);
        free(queue);
        return NULL;
    }

    return queue;
}

bool enqueue_frame(frame_queue *queue, struct frame *frame) {

    //queue is full, should not even be called if this is the case
    if (queue->size == FRAME_QUEUE_CAPACITY) {
        return false;
    }

    queue->frames[queue->rear] = frame;
    queue->rear = (queue->rear + 1) % FRAME_QUEUE_CAPACITY;
    queue->size++;

    SDL_SignalCondition(queue->not_empty);
    return true;
}

struct frame *dequeue_frame(frame_queue *queue) {

    // if queue is empty, shouldn't even be called if this is the case
    if (queue->size == 0) {
        return NULL;
    }

    struct frame *frame = queue->frames[queue->front];
    queue->front = (queue->front + 1) % FRAME_QUEUE_CAPACITY;
    queue->size--;

    SDL_SignalCondition(queue->not_full);
    return frame;
}

void destroy_frameQueue(frame_queue *queue) {
    if (!queue) return;

    SDL_LockMutex(queue->mutex); //TODO idk if this is needed, time will come

    // free all frames in the queue
    for (int i = 0; i < FRAME_QUEUE_CAPACITY; i++) {
        if (queue->frames[i]) {
            destroy_frame(queue->frames[i]);
        }
    }

    SDL_UnlockMutex(queue->mutex); //TODO here as well
    SDL_DestroyMutex(queue->mutex);

    SDL_DestroyCondition(queue->not_empty);
    SDL_DestroyCondition(queue->not_full);
    free(queue);
}
