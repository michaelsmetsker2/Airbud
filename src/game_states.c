/**
* @file game_states.c
 *
 * definitions of games states and what buttons they contain and a function to change between them
 * start and end byte offsets are set as the chunk value then multiplied by bytes per chunk at compile time,
 * this is to lessen confusion over a potentially mis-inputted value
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <game_states.h>
#include <read_file.h>
#include <game_logic.h>


#define BYTES_PER_CHUNK 2048

bool change_game_state(app_state *appstate, const STATE_ID destination) {

    // waits for decode loop to exit cleanly
    SDL_SetAtomicInt(&appstate->stop_decoder_thread, 1);
    SDL_LockMutex(appstate->playback_instructions->mutex);

    // waits for render loop to exit cleanly
    SDL_SetAtomicInt(&appstate->stop_render_thread, 1);
    SDL_LockMutex(appstate->renderer_mutex);

    // sets audio samples to zero and clears audio stream
    SDL_SetAtomicU32(&appstate->total_audio_samples, 0);
    SDL_ClearAudioStream(appstate->audio_stream);

    // clears frame queue
    clear_frame_queue(appstate->render_queue);

    // changes main thread gamestate
    appstate->current_game_state = &GAME_STATES[destination];

    // updates decoding instructions
    appstate->playback_instructions->start_offset_bytes = appstate->current_game_state->start_offset_bytes;
    appstate->playback_instructions->end_offset_bytes = appstate->current_game_state->end_offset_bytes;
    appstate->playback_instructions->audio_only = appstate->current_game_state->audio_only;

    //TODO conditionally run the pre commands

    // resumes threads
    SDL_UnlockMutex(appstate->playback_instructions->mutex);
    SDL_UnlockMutex(appstate->renderer_mutex);

    return true;
}

const struct game_state GAME_STATES[STATE_COUNT] = {
    [MAIN_MENU_1] = {
        .start_offset_bytes = 0 * BYTES_PER_CHUNK,
        .end_offset_bytes = 6462 * BYTES_PER_CHUNK,
        .audio_only = false,
        .pre_commands = NULL,
        .next_state = next_MAIN_MENU_1,
        .buttons_count = 4,
    },
    [MAIN_MENU_2] = {
        .start_offset_bytes = 6463 * BYTES_PER_CHUNK,
        .end_offset_bytes = 6761 * BYTES_PER_CHUNK,
        .audio_only = true,
        .next_state = next_MAIN_MENU_2,
    },
    [MAIN_MENU_3] = {
        .start_offset_bytes = 6762 * BYTES_PER_CHUNK,
        .end_offset_bytes = 7296 * BYTES_PER_CHUNK,
        .audio_only = true,
        .next_state = next_MAIN_MENU_3,
    },
    [TUTORIAL] = {
        .start_offset_bytes = 7297 * BYTES_PER_CHUNK,
        .end_offset_bytes = 24292 * BYTES_PER_CHUNK,
        .audio_only = false,
    },
};
