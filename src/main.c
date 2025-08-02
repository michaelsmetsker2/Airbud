/**
* @file init.c
 *
 * Main loop for the airbud dvd bonus game
 *
 * @author Michael Metsker
 * @version 1.0
 */

//TODO end whole program when a single thread errors out

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

#include <render.h>
#include <init.h>

/* runs on startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) { //TODO add usage or sumthin? play me twerkin

    *appstate = initialize();
    if (*appstate == NULL) {
        return SDL_APP_FAILURE;
    }

    if (!start_threads(*appstate)) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; /* carry on with the program!*/
}

/* This function runs once per frame, main loop*/
SDL_AppResult SDL_AppIterate(void *appstate) {

    render_frame(appstate);

    //Render layers

    //TODO menu layer/buttons

    return SDL_APP_CONTINUE;
}

/* Runs when a new event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button) {
            SDL_Log("clicked!");
        }
    }

    //TODO press f to go fullscreen

    return SDL_APP_CONTINUE;
}

/* Runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */

    //TODO clean up framequeue
    //SDL_DestroyAudioStream
}
