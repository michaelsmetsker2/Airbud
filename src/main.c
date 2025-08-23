/**
* @file init.c
 *
 * Main loop for the airbud dvd bonus game
 *
 * @author Michael Metsker
 * @version 1.0
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

#include <init.h>

#include "game_logic.h"

/* runs on startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) { //TODO add usage

    *appstate = initialize();
    if (*appstate == NULL) {
        return SDL_APP_FAILURE;
    }

    if (!start_threads(*appstate)) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; /* carry on with the program!*/
}

/* This required function runs once per frame, main loop*/
SDL_AppResult SDL_AppIterate(void *appstate) {

    return SDL_APP_CONTINUE;
}

/* Runs when an event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button) {
            SDL_Log("clicked!");

            change_game_state(appstate, TUTORIAL);

            /* todo
            if click is on a button {
                do stuff :)
            }
             */

        }
    }

    //TODO press f to go fullscreen

    return SDL_APP_CONTINUE;
}

/* Runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */

    //TODO end whole program when a single thread errors out

    //TODO clean up framequeue
    //SDL_DestroyAudioStream
}
