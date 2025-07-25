/**
* @file init.c
 *
 * contains implementations for all initialization functions related to ffmpeg and sld3
 *
 * @author Michael Metsker
 * @version 1.0
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

#include <render.h>
#include "../include/init.h"


#include <windows.h>
#include <psapi.h>

void print_memory_usage() {
    PROCESS_MEMORY_COUNTERS memInfo;
    GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
    printf("Memory used: %zu KB\n", memInfo.WorkingSetSize / 1024);
}

/* runs on startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) { //TODO add cli easter egg or something?

    *appstate = initialize();
    if (*appstate == NULL) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; /* carry on with the program!*/
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    const app_state *state = (app_state *) appstate;

    render_frame(state);

    //Render layers

    //TODO menu layer/buttons

    print_memory_usage(); //TODO debug
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */
    //TODO clean up framequeue
}
