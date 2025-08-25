/**
 * @file frame_queue.h
 *
 * Contains a memory safe queue of av frames
 * that will be accessible to multiple threads simultaneously
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

/**
 * @struct frame_queue
 * @brief memory safe queue of AVFrames
 */
typedef struct frame_queue {
    AVFrame **frames;         /**< Dynamic circular buffer of frame pointers */
    int capacity;             /**< Max capacity of the array */
    int size;                 /**< Current number of frames in the queue */
    int front;                /**< Index of first frame */
    int rear;                 /**< Index of last frame */

    SDL_Mutex *mutex;         /**< Guards access from multiple threads to prevent data corruption */
    SDL_Condition *not_empty; /**< Signaled when frames are added */
    SDL_Condition *not_full;  /**< Signaled when frames are removed */
} frame_queue;

/**
 * @brief creates and populates a frame queue
 * @return *frame_queue - pointer to the created queue
 */
frame_queue *create_frame_queue();

/**
 * @brief takes a copy of a frame and sends it to the queue
 * does not internally handle mutex
 *
 * @param queue queue to be added to
 * @param frame AVFrame to clone and add to the queue
 * @return true on success false on failure
 */
bool enqueue_frame(frame_queue *queue, AVFrame *frame);

/**
 * @brief pops the first frame in the given queue
 * does not internally handle mutex
 *
 * @param queue queue to pop from
 * @return frame* - pointer to the AVFrame that has been popped or NULL if empty
 */
AVFrame *dequeue_frame(frame_queue *queue);

/**
 * @brief clears the given frame_queue
 * internally handles mutex
 *
 * @param queue pointer to queue to clear
 */
void clear_frame_queue(frame_queue *queue);

/**
 * @brief destroys a frame_queue freeing all associated resources
 * internally handle mutex
 *
 * @param queue queue to be destroyed
 */
void destroy_frameQueue(frame_queue *queue);

#endif //FRAME_QUEUE_H
