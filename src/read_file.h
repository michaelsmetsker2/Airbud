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
#include <init.h>

/**
 * @struct decoder_thread_args
 * @brief Parameters for the decoder thread.
 */
struct decoder_thread_args {
    SDL_AtomicInt *exit_flag;           /**< 0, 1 whether the thread should stop executing */

    frame_queue *video_queue;           /**< video queue to add frames to */
    SDL_AudioStream *audio_stream;      /**< audio stream for sound playback */

    const char *filename;               /**< file to be played back */
};

/**
 * @brief
 * @param appstate copies references to various variables from appstate into decoder_args
 * @param filename name of the file to read from
 */
struct decoder_thread_args *create_decoder_args(app_state *appstate, const char *filename);
// TODO do i have a way to clean up args?

/**
 * @brief a thread that manages decoding frames and audio from a file, then adds decoded data to a frame queue
 *
 * @param data pointer to decoder_args struct
 * @return 0 if clean shutdown
 */
int play_file(void *data);

//TODO add a switch file function
// REMEMBER to set audio playback time to zero

#endif //READ_FILE_H
