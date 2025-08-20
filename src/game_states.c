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
        .id = MAIN_MENU_1, //FIXME potentially redundant?
        .chunk_offset = 0,
        .end_pts = 9999999,
        .audio_only = false,
        // .buttons = main_menu_buttons
    },
    [MAIN_MENU_2] = {
        .id = MAIN_MENU_1, //FIXME potentially redundant?
        .chunk_offset = 6463,
        .end_pts = 9999999,
        .audio_only = true,
        // .buttons = main_menu_buttons
    },
    [MAIN_MENU_3] = {
        .id = MAIN_MENU_1, //FIXME potentially redundant?
        .chunk_offset = 6762,
        .end_pts = 9999999,
        .audio_only = true,
        // .buttons = main_menu_buttons
    },
};
