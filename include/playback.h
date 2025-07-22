/**
 * @file playback.h
 *
 * handles decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
*/

#ifndef DECODE_H
#define DECODE_H

#include "common.h"
#include "../include/frame-queue.h"

/**
 * @struct PlaybackArgs
 *
 * Parameters for the AVPlayback thread.
*/
typedef struct {
    volatile bool *shouldExit; /**< default false, whether the thread should stop executing */
    const char *filename;     /**< file to be played back */
    FrameQueue *queue;        /**< queue to add frames to */
} PlaybackArgs;

/**
 * manages decoding frames and audio from a file, then adds decoded data to a frame queue
 *
 * @param data pointer to PlaybackArgs struct
 * @return 0 if clean shutdown -1 if error
*/
int AVPlayback(void *data);

/**
 * TODO
 *
*/
bool attemptEnqueue();

//TODO will prolly need a switch file function

#endif //DECODE_H
