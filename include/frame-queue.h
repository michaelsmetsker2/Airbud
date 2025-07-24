/**
 * @file frame-queue.h
 *
 * Contains a memory safe queue of av frames
 * that will be accessible to multiple threads simultaneously
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include "common.h"
#include <libavutil/frame.h>

/** the max amount of frames to buffer */
#define FRAME_QUEUE_CAPACITY 8

/**
 * @struct frame
 * @brief contains audio video and button data // TODO outdated
 */
struct frame {
    AVFrame *video_frame;    /**< AVFrame that stores the video data */
    AVFrame *audio_frame;    /**< AVFrame that stores the audio data */ //TODO get rid of this
    //menu_button buttons[]; /**< stores an array of all currently active buttons */ //TODO menu buttons definition needed
};

/**
 * @brief memory safe deletion of frame struct
 * @param frame pointer to frame that will be destroyed
 */
void destroy_frame(struct frame *frame);

/**
 * @struct frame_queue
 * @brief memory safe queue of 16 frames
 */
typedef struct frame_queue {
    struct frame *frames[FRAME_QUEUE_CAPACITY];  /**< Circular buffer of frame pointers */
    int size;                                    /**< Current number of frames in the queue */
    int front;                                   /**< Index of first frame */
    int rear;                                    /**< Index of last frame */

    SDL_Mutex *mutex;                            /**< Guards access from multiple threads to prevent data corruption */
    SDL_Condition *not_empty;                    /**< Signaled when frames are added */
    SDL_Condition *not_full;                     /**< Signaled when frames are removed */
} frame_queue;

/**
 * @brief creates and populates a frame queue
 * @return *frame_queue - pointer to the created queue
 */
frame_queue *create_frame_queue();

/**
 * @brief adds a frame to end of the queue
 *
 * does not internally handle mutex
 *
 * @param queue queue to be added to
 * @param frame frame to be added to the queue
 * @return true on success false on failure
 */
bool enqueue_frame(frame_queue *queue, struct frame *frame);

/**
 * @brief pops the first frame in the given queue
 *
 * does not internally handle mutex
 *
 * @param queue queue to pop from
 * @return Frame* - pointer to the frame that has been popped or NULL if empty
 */
struct frame *dequeue_frame(frame_queue *queue);

/**
 * @brief destroys a frame_queue freeing all associated resources
 *
 * CURRENTLY DOES INTERNALLY HANDLE MUTEX
 *
 * @param queue queue to be destroyed
 */
void destroy_frameQueue(frame_queue *queue);

#endif //FRAME_QUEUE_H
