/**
 * @file playback.h
 *
 * handles decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <common.h>
#include <frame-queue.h>

/**
 * @struct playback_args
 * @brief Parameters for the decoder thread.
 */
struct playback_args {
    volatile bool *exit_flag; /**< default false, whether the thread should stop executing */
    const char *filename;     /**< file to be played back */
    frame_queue *queue;       /**< queue to add frames to */
};

/**
 * @brief manages decoding frames and audio from a file, then adds decoded data to a frame queue
 *
 * @param data pointer to playback_args struct
 * @return 0 if clean shutdown
 */
int play_file(void *data);

//TODO will prolly need a switch file function

#endif //PLAYBACK_H
