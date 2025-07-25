/**
 * @file init.c
 *
 * functions for initialization and populating critical structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <common.h>
#include <init.h>
#include <playback.h>

app_state *initialize() {
    SDL_SetAppMetadata("airbud", "1.0", "com.airbud.renderer");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    // Creates empty app_state struct
    app_state *state = malloc(sizeof(app_state));
    if (!state) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return NULL;
    }

    // Creates window and renderer and adds them to state
    if (!SDL_CreateWindowAndRenderer("airbud/renderer", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return NULL;
    }

    // Creates a frame_queue for the app
    state->queue = create_frame_queue(VIDEO_BUFFER_CAP);
    if (!state->queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frame_queue\n");
        return NULL;
    }

    // Creates decoder thread exit flag and sets it to false
    state->stop_decoder_thread = calloc(1, sizeof(bool));
    if (!state->stop_decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder exit flag\n");
        return NULL;
    }

    //create playback args for decoder thread and populate it //TODO could move to new function?
    struct playback_args *args = malloc(sizeof(struct playback_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for decoder thread\n");
        return NULL;
    }
    args->exit_flag = state->stop_decoder_thread;
    args->queue = state->queue;
    args->filename = TEST_FILE_URL;

    state->decoder_thread = SDL_CreateThread(play_file, "decoder", args);
    if (!state->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder thread\n");
        return NULL;
    }

    return state;
}