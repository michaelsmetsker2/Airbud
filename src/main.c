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

#include "../include/common.h"
#include "../include/init.h"
#include "../include/playback.h"

const char TEST_FILE_URL[] = "Z:/projects/airbud/VTS_01_1.VOB"; //TODO replace with actual file list

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

// runs on startup
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) //TODO cli integration? maybe an easter egg?
{
    if (!sdl_init(&window, &renderer)) {
        return SDL_APP_FAILURE;
    } // Initialize SDL

    FrameQueue *frameQueue = createFrameQueue(); // Queue of buffered frames
    if (!frameQueue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frameQueue\n");
        return SDL_APP_FAILURE;
    }

    volatile bool *decoder_exit_flag = malloc(sizeof(bool)); //Exit flag for decoder thread
    if (!decoder_exit_flag) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate exit flag\n");
        return SDL_APP_FAILURE;
    }

    PlaybackArgs *args = malloc(sizeof(PlaybackArgs));
    args->shouldExit = decoder_exit_flag;
    args->filename = TEST_FILE_URL;
    args->queue = frameQueue;
    SDL_Thread *playbackThread = SDL_CreateThread(AVPlayback, "playback", args);

    //create the appstate struct
    Appstate *state = malloc(sizeof(Appstate));
    if (!appstate) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return SDL_APP_FAILURE;
    }
    //state->shouldExit = DecoderExitFlag;
    state->queue = frameQueue;
    state->stopBuffering = decoder_exit_flag;

    *appstate = state;

    return SDL_APP_CONTINUE; /* carry on with the program!*/
}

/* This function runs when a new event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    Appstate *state = (Appstate *) appstate;

    SDL_LockMutex(state->queue->mutex);
    if (state->queue->size > 0) {
        // frame in the queue
    }
    SDL_UnlockMutex(state->queue->mutex);

    //jsut make it debug that shit atm

    //TODO remove all this and actually render the frames
    const double now = ((double) SDL_GetTicks()) / 1000.0; /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    const float red = (float) (0.5 + 0.5 * SDL_sin(now));
    const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT); /* new color, full alpha. */


    //TODO menu layer/buttons

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */
    //TODO clean up framequeue
}
