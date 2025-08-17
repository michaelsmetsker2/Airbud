/**
 * @file render.h
 *
 * Functions relating to displaying video and syncing it to audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#ifndef RENDER_H
#define RENDER_H

#include <init.h>
#include <stdbool.h>

/**
 * @struct render_thread_args
 * @brief Struct containing nesesary information for the render thread
 * although these are just a subset of appstate, this allows for more encapsulization
 */
struct render_thread_args {
    SDL_AtomicInt *exit_flag;           /**< exit flag for safe quick exit */

    SDL_Window *window;                 /**< main window for the app */
    SDL_Renderer *renderer;             /**< main renderer for the app */
    SDL_Texture *texture;               /**< reused texture to avoid repeate declarations and memory churn */

    frame_queue *queue;                 /**< queue of avframes to render */
    SDL_AtomicU32 *total_audio_samples; /**< total amount of audio samples pushed to the audio queue, used for syncing */
    SDL_AtomicU32 *audio_offset_ms;     /**< audio offset for starting midway through a file */
    SDL_AudioStream *audio_stream;      /**< audio stream where audio packets are queued */

};

/**
 * @brief creates render thread with the correct parameters
 * @return true on success, false otherwise
 */
bool create_render_thread(app_state *appstate);

/**
 * @brief thread that manages rendering
 * //TODO update for render layers?
 *
 * @param data pointer to a render_thread_args struct
 * @return 0 on clean shutdown
 */
 int render_frames(void *data);

#endif //RENDER_H
