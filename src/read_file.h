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
#include <stdbool.h>
#include <stdint.h>

/**
 * @struct decoder_instructions
 * @brief contains mutex controlled variables that change when the gamestate us updated.
 * these are subsets of the game_state struct and should only be changed from the main thread when changing gamestate
 */
struct decoder_instructions {

    bool audio_only;                        /**< whether the next section only needs decoded audio */
    uint32_t start_offset_bytes;            /**< the point in the file to start decoding from */
    uint32_t end_offset_bytes;              /**< the end of the current chunk */

    SDL_Mutex *mutex;                       /**< mutex will be held by decoder until its exit flag is triggered */

    SDL_AtomicInt end_reached;              /**< mutex independent flag triggered when the decoder has reached the end of the instructed bytes */
};

// TODO make function to cleanup decoder_instructions

/**
 * @brief Creates and starts the decoder thread with the correct parameters starts it
 * this populates the passed appstates struct's playback instructions, decoder thread, and stop_decoder_thread members
 * clean exit is forced by setting the stop decoder thread flag to -1
 * @param appstate copies references to various variables from appstate into decoder_thread_args
 * @return true on success false otherwise
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
