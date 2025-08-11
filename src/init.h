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

    SDL_AudioStream    *audio_stream;         /**< audio stream for sound playback */
    SDL_AtomicU32       total_audio_samples;  /**< total amount of packets of audio enqueued, used for syncing renderer */

    frame_queue        *render_queue;         /**< render queue of buffered video frames */

    SDL_Thread         *render_thread;        /**> Thread that handles rendering */
    SDL_AtomicInt       stop_render_thread;   /**> The exit flag for the render thread */

    SDL_Thread         *decoder_thread;       /**< pointer to the thread that handles decoding */
    SDL_AtomicInt       stop_decoder_thread;  /**< The exit flag for the decoding thread */

} app_state;

/**
 * @brief Initializes SDL and allocates the app_state struct
 *
 * @return *app_state - pointer to an app_state struct containing critical SDL resources, or NULL on failure
 */
app_state *initialize();

/**
 * @brief starts decoding thread and sdl audio stream with the correct parameters
 *
 * @param appstate app state containing various app wide variables
 * @return true on success, false otherwise
 */
bool start_threads(app_state *appstate);

#endif //INIT_H
