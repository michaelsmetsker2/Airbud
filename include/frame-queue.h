/**
 * @file frame-queue.h
 *
 * Contains a memory safe queue of av frames //todo genericize for audio as well
* that will be accessible to multiple threads simultaneously
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

/** the max amount of frames to buffer / hold in a queue */
static const int VIDEO_BUFFER_CAP = 5;
/** the max amount of audio frames to buffer **/
static const int AUDIO_BUFFER_CAP = 40;

/**
 * @struct frame_queue
 * @brief memory safe queue of 16 frames
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
 * @param capacity Size of the array, should be set with either VIDEO_BUFFER_CAP or AUDIO_BUFFER_CAP
 * @return *frame_queue - pointer to the created queue
 */
frame_queue *create_frame_queue(const int capacity);

/**
 * @brief adds a frame to end of the queue
 *
 * ON FAILURE THIS DOES NOT FREE THE FRAME
 * does not internally handle mutex
 *
 * @param queue queue to be added to
 * @param frame AVFrame to be added to the queue
 * @return true on success false on failure
 */
bool enqueue_frame(frame_queue *queue, AVFrame *frame);

/**
 * @brief pops the first frame in the given queue
 *
 * does not internally handle mutex
 *
 * @param queue queue to pop from
 * @return frame* - pointer to the AVFrame that has been popped or NULL if empty
 */
AVFrame *dequeue_frame(frame_queue *queue);

/**
 * @brief destroys a frame_queue freeing all associated resources
 *
 * CURRENTLY DOES INTERNALLY HANDLE MUTEX
 *
 * @param queue queue to be destroyed
 */
void destroy_frameQueue(frame_queue *queue);

#endif //FRAME_QUEUE_H
