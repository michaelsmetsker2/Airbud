/**
 * @file frame-queue.h
 *
 * Contains a (hopefully) memory safe way of adding av frames to a queue
 * that will be accessible to multiple threads simultaneously
 *
 * @author Michael Metsker
 * @version 1.0
*/
#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include "common.h"
#include <libavutil/frame.h>

#define FRAME_QUEUE_CAPACITY 16 // the max amount of frames to buffer


/**
 * @struct Frame
 *
 * contains audio video and button data
*/
typedef struct {
    AVFrame *videoFrame; /**< AVFrame that stores the video data */
    AVFrame *audioFrame; /**< AVFrame that stores the audio data */
    //menuButton buttons[]; /**< stores an array of all currently active buttons */ //TODO menu buttons definition needed
} Frame;

/**
 * memory safe deletion of Frame struct
 *
 * @param frame pointer to Frame that will be destroyed
*/
void destroyFrame(Frame *frame);

/**
 * @struct FrameQueue
 *
 * memory safe queue of frames 16 Frames other relevant data for rendering
*/
typedef struct {
    Frame *frames[FRAME_QUEUE_CAPACITY];  /**< Circular buffer of Frame pointers */
    int size;                             /**< Current number of frames in the queue */
    int front;                            /**< Index of first Frame */
    int rear;                             /**< Index of last Frame */

    SDL_Mutex *mutex;                     /**< guards access from multiple threads to prevent data corruption */
    SDL_Condition *notEmpty;              /**< Signaled when Frames are added */
    SDL_Condition *notFull;               /**< Signaled when Frames are removed */
} FrameQueue;

/**
 * Creates a frame queue
 *
 * @return FrameQueue - pointer to the created queue
 */
FrameQueue *createFrameQueue();

/**
 * adds a Frame to the queue
 *
 * @param queue queue to be added to
 * @param frame frame to be added to the queue
 * @return bool - true on success false on failure
 */
bool enqueueFrame(FrameQueue *queue, Frame *frame);

/**
 * pops the first frame in the given queue
 *
 * @param queue queue to pop from
 * @return Frame* - pointer to the frame that has been popped or NULL if empty
 */
Frame *dequeueFrame(FrameQueue *queue);

/**
 * destroys a FrameQueue freeing all associated resources
 *
 * @param queue queue to be destroyed
 */
void destroyFrameQueue(FrameQueue *queue);

#endif //FRAME_QUEUE_H
