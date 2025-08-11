//
// Created by micha on 8/9/2025.
//

#include <game_states.h>

const struct game_state game_states[3] = {
    [MAIN_MENU_1] = {
        .start_pts = 0,
        .end_pts = 10000,
        .audio_only = false,
        // .buttons = main_menu_buttons
    },
};
