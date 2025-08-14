/**
* @file game_states.c
 *
 * definitions of games states and buttons
 *
 * @author Michael Metsker
 * @version 1.0
 */
#include <game_states.h>

const struct game_state GAME_STATES[4] = {
    [MAIN_MENU_1] = {
        .start_pts = 800000,
        .end_pts = 20000,
        .audio_only = false,
        // .buttons = main_menu_buttons
    },
};
