/**
 * @file game_states.h
 *
 * Hardcoded games states and their button positions
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef GAME_STATES_H
#define GAME_STATES_H

#include <stdint.h> //TODO make this the standard
#include <stdbool.h>

#define STATE_COUNT 4

typedef enum STATE_ID {
    MAIN_MENU_1,
    MAIN_MENU_2,
    MAIN_MENU_3,
    TUTORIAL
} STATE_ID;

/**
 * TODO
 *
 *
 */
struct game_state {
    const STATE_ID id;
    const uint32_t chunk_offset;
    const uint32_t end_pts;
    const bool audio_only;

    //TODO string of buttons
    //TODO potentially references to logic functions? of this stuff?
};

extern const struct game_state GAME_STATES[STATE_COUNT];



#endif //GAME_STATES_H
