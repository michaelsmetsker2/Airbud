/**
 * @file init.h
 *
 * Contains basic initialization functions for SD3
 * and appstate struct to carry data between sdl functions
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef INIT_H
#define INIT_H

#include <frame_queue.h>
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * @struct app_state
 * @brief Struct for carrying basic info to all parts of the SDL program
 */
typedef struct app_state {
    SDL_Window         *window;               /**< main Window for the program */
    SDL_Renderer       *renderer;             /**< main Renderer for the program */
    SDL_Texture        *base_texture;         /**< Reused texture for main video playback */

    frame_queue        *video_queue;          /**< render queue of buffered video frames */
    frame_queue        *audio_queue;          /**< render queue of buffered audio frames */

    SDL_Thread         *decoder_thread;       /**< pointer to the thread that handles decoding */
    volatile bool      *stop_decoder_thread;  /**< The exit flag for the decoding thread */

    SDL_Thread         *audio_thread;         /**< handles audio playback */
    volatile bool      *stop_audio_thread;    /**< exit flag for audio thread */

    SDL_AudioStream    *audio_stream;         /**< audio stream for sound playback */

    /* TODO game_state??? potentially idk */
} app_state;

/**
 * @brief Initializes SDL and allocates the app_state struct
 *
 * @return *app_state - pointer to an app_state struct containing critical SDL resources, or NULL on failure
 */
app_state *initialize();

/**
 * @brief starts audio and decoding threads with the correct parameters
 *
 * @param appstate app state containing various app wide variables
 * @return true on success, false otherwise
 */
bool start_threads(app_state *appstate);

#endif //INIT_H
