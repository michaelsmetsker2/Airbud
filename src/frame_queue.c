/**
 * @file frame_queue.c
 *
 * helper functions for frame and frame_queue structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

#include <stdbool.h>

#include <frame_queue.h>

/** the max amount of frames to buffer / hold in a queue */
static const int VIDEO_BUFFER_CAP = 32;

frame_queue *create_frame_queue() {
    frame_queue *queue = malloc(sizeof(frame_queue));
    if (!queue) return NULL;

    queue->capacity = VIDEO_BUFFER_CAP;
    queue->frames = malloc(sizeof(AVFrame*)* queue->capacity);
    if (!queue->frames) {
        free(queue);
        return NULL;
    }

    queue->size = 0;
    queue->front = 0;
    queue->rear = 0;

    queue->mutex = SDL_CreateMutex();
    queue->not_empty = SDL_CreateCondition();
    queue->not_full = SDL_CreateCondition();

    if (!queue->mutex || !queue->not_empty || !queue->not_full) {
        SDL_DestroyMutex(queue->mutex);
        SDL_DestroyCondition(queue->not_empty);
        SDL_DestroyCondition(queue->not_full);
        free(queue->frames);
        free(queue);
        return NULL;
    }
    return queue;
}

bool enqueue_frame(frame_queue *queue, AVFrame *frame) {

    //queue is full, should not even be called if this is the case
    if (queue->size == queue->capacity) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "can't queue frame, queue is full\n");
        return false;
    }

    //clone the frame
    AVFrame *frame_copy = av_frame_clone(frame);
    if (!frame_copy) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame for queueing\n");
        return false;
    }

    queue->frames[queue->rear] = frame_copy;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;

    SDL_SignalCondition(queue->not_empty);
    return true;
}

AVFrame *dequeue_frame(frame_queue *queue) {

    // if queue is empty, shouldn't even be called if this is the case
    if (queue->size == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "can't dequeue frame, queue is empty\n");
        return NULL;
    }

    AVFrame *frame = queue->frames[queue->front];
    queue->frames[queue->front] = NULL;

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    SDL_SignalCondition(queue->not_full);
    return frame;
}

void destroy_frameQueue(frame_queue *queue) {
    if (!queue) return;

    SDL_LockMutex(queue->mutex);

    // free all frames in the queue
    for (int i = 0; i < queue->capacity; i++) {
        if (queue->frames[i]) {

            av_frame_free(&queue->frames[i]);
        }
    }

    SDL_UnlockMutex(queue->mutex);

    SDL_DestroyMutex(queue->mutex);
    SDL_DestroyCondition(queue->not_empty);
    SDL_DestroyCondition(queue->not_full);

    free(queue->frames);
    free(queue);
}