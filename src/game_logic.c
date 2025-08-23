/**
* @file game_logic.c
 *
 * Logic for gameplay, buttons and switching between game states
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include "game_logic.h"
#include <SDL3/SDL.h>

#include "read_file.h"

bool change_game_state(app_state *appstate, const STATE_ID destination) {

    // waits for decode loop to exit cleanly
    SDL_SetAtomicInt(&appstate->stop_decoder_thread, 1);
    SDL_LockMutex(appstate->playback_instructions->mutex);

    // waits for render loop to exit cleanly
    SDL_SetAtomicInt(&appstate->stop_render_thread, 1);
    SDL_LockMutex(appstate->renderer_mutex);
    printf("debug");

    // resets audio clock
    SDL_SetAtomicU32(&appstate->total_audio_samples, 0);
    
    // FIXME conditionaly clear render queues?

    appstate->current_game_state = &GAME_STATES[destination];

    appstate->playback_instructions->start_offset_bytes = appstate->current_game_state->start_offset_bytes;
    appstate->playback_instructions->end_offset_bytes = appstate->current_game_state->end_offset_bytes;
    appstate->playback_instructions->audio_only = appstate->current_game_state->audio_only;

    SDL_UnlockMutex(appstate->playback_instructions->mutex);
    SDL_UnlockMutex(appstate->renderer_mutex);

    return true;
}
