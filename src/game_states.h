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

/**
 * TODO
 *
 *
 */
struct game_state {
    const uint32_t start_pts;
    const uint32_t end_pts;
    const bool audio_only;

    //TODO string of buttons
};

typedef enum STATE_ID {
    MAIN_MENU_1,
    MAIN_MENU_2,
    MAIN_MENU_3,
    TUTORIAL
} STATE_ID;

extern const struct game_state GAME_STATES[STATE_COUNT];



#endif //GAME_STATES_H
