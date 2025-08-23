/**
 * @file game_states.h
 *
 * Hardcoded games states and their buttons
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef GAME_STATES_H
#define GAME_STATES_H

#include <stdint.h> //TODO make this the standard
#include <stdbool.h>

#define STATE_COUNT 4 //FIXME, this polutes the namespace?

/**
 * TODO
 *
 *
 */
typedef struct button {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    void (*on_click)();

} button;


typedef enum STATE_ID {
    MAIN_MENU_1,
    MAIN_MENU_2,
    MAIN_MENU_3,
    TUTORIAL
} STATE_ID;

/**
 * @struct game_state
 * TODO
 *
 *
 */
struct game_state {
    const uint32_t start_offset_bytes;
    const uint32_t end_offset_bytes;
    const bool audio_only;              /** if the corosponding section of the file only has audio information */

    const STATE_ID next_state;

    const button *buttons;
    const uint8_t buttons_count;

    //TODO next sequential chunk?? maybe have 0 be for looping
    //TODO potentially references to logic functions? of this stuff?
};

extern const struct game_state GAME_STATES[STATE_COUNT];



#endif //GAME_STATES_H
