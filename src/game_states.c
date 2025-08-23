/**
* @file game_states.c
 *
 * definitions of games states and buttons
 * start and end byte offsets are set as the chunk value then multiplied by bytes per chunk at compile time,
 * this is to lessen confusion over a potentially mis-inputted value
 *
 * @author Michael Metsker
 * @version 1.0
 */
#include <game_states.h>

#define BYTES_PER_CHUNK 2048

const struct game_state GAME_STATES[STATE_COUNT] = {
    [MAIN_MENU_1] = {
        .start_offset_bytes = 0 * BYTES_PER_CHUNK,
        .end_offset_bytes = 6462 * BYTES_PER_CHUNK,
        .audio_only = false,
    },
    [MAIN_MENU_2] = {
        .start_offset_bytes = 6463 * BYTES_PER_CHUNK,
        .end_offset_bytes = 6761 * BYTES_PER_CHUNK,
        .audio_only = true,
    },
    [MAIN_MENU_3] = {
        .start_offset_bytes = 6762 * BYTES_PER_CHUNK,
        .end_offset_bytes = 7296 * BYTES_PER_CHUNK,
        .audio_only = true,
    },
    [TUTORIAL] = {
        .start_offset_bytes = 7297 * BYTES_PER_CHUNK,
        .end_offset_bytes = 24292 * BYTES_PER_CHUNK,
        .audio_only = false,
    },
};
