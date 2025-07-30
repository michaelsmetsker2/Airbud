/**
* @file init.c
 *
 * contains implementations for all initialization functions related to ffmpeg and sld3
 * @author Michael Metsker
 * @version 1.0
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

#include <render.h>
#include <init.h>

/* runs on startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) { //TODO add cli easter egg or something?

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

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* Runs when a new event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* Runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */
    //TODO clean up framequeue
}
