/**
 * @file read_file.h
 *
 * Main file for the decoder thread.
 * Handles opening files, decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef READ_FILE_H
#define READ_FILE_H

#include <frame_queue.h>
#include <stdbool.h>
#include <init.h>

/**
 * @struct decoder_thread_args
 * @brief Parameters for the decoder thread.
 */
struct decoder_thread_args {
    volatile bool *exit_flag;       /**< default false, whether the thread should stop executing */
    const char *filename;           /**< file to be played back */
    frame_queue *video_queue;       /**< video queue to add frames to */
    frame_queue *audio_queue;       /**< audio queue to add frames to */
};

/**
 * @brief
 * @param appstate copies references to various variables from appstate into decoder_args
 * @param filename name of the file to read from
 */
struct decoder_thread_args *create_decoder_args(const app_state *appstate, const char *filename);
// TODO do i have a way to clean up args?


/**
 * @brief a thread that manages decoding frames and audio from a file, then adds decoded data to a frame queue
 *
 * @param data pointer to decoder_args struct
 * @return 0 if clean shutdown
 */
int play_file(void *data);

//TODO add a switch file function

#endif //READ_FILE_H
