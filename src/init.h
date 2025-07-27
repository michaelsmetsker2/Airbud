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

#include <frame-queue.h>
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * @struct app_state
 * @brief Struct for carrying basic info to all parts of the SDL program
 */
typedef struct app_state {
    SDL_Window      *window;                /**< Main Window for the program */
    SDL_Renderer    *renderer;              /**< Main Renderer for the program */
    SDL_Texture     *base_texture;          /**< Reused texture for main video playback */

    SDL_Thread      *decoder_thread;        /**< Pointer to the thread that handles decoding */
    volatile bool   *stop_decoder_thread;   /**< The exit flag for the decoding thread */

    frame_queue     *video_queue;                 /**< render queue of buffered video frames */
    frame_queue     *audio_queue;                 /**< render queue of buffered audio frames */

    /* TODO game_state??? potentially idk */
} app_state;

/**
 * @brief Initializes SDL and allocates the app_state struct
 *
 * This will also start the decoding thread on the starting file
 *
 * @return *app_state - pointer to an app_state struct containing critical SDL resources, or NULL on failure
 */
app_state *initialize();

#endif //INIT_H
