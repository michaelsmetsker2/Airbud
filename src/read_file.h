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

#include <init.h>

/**
 * TODO
 */
struct decoder_instructions {

    const void *state; //atomic pointer

    SDL_Mutex *mutex;
    SDL_Condition *instruction_available;
};

/**
 * @brief Creates and starts the decoder thread with the correct parameters TODO and starts it off at the beginning of the app?
 * clean exit is forced by pushing a null gamestate as instructions
 * @param appstate copies references to various variables from appstate into decoder_thread_args
 */
bool create_decoder_thread(app_state *appstate);
// TODO do i have a way to clean up args?

/**
 * @brief a thread that manages decoding frames and audio from a file, then adds decoded data to a frame queue
 *
 * @param data pointer to decoder_args struct
 * @return 0 if clean shutdown
 */
int play_file(void *data);

#endif //READ_FILE_H
