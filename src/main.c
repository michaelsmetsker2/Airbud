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

/* required function that runs once per frame */
SDL_AppResult SDL_AppIterate(void *appstate) {
    return SDL_APP_CONTINUE;
}

/* Runs when an event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    const app_state *state = appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            // X button clicked
            return SDL_APP_SUCCESS;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button) {
                SDL_Log("clicked!");

                change_game_state(appstate, MAIN_MENU_2);

                /* todo
                for each button in the current state
                    if click is on a button {
                        do stuff :)
                    }
                */
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_F) {
                // toggle fullscreen
                const Uint32 flags = SDL_GetWindowFlags(state->window);

                if (flags & SDL_WINDOW_FULLSCREEN) {
                    // currently fullscreem, changes to windowed
                    SDL_SetWindowFullscreen(state->window, false);
                } else {
                    // currently windowed, changes to fullscreen
                    SDL_SetWindowFullscreen(state->window, true);
                }
            }
            break;
        default:

            if (event->type == state->decoding_ended_event) {
                SDL_Log("signal read by main thread \n");

                //audio and video queues will inherently be clear when this is called, this wastes time double clearing them
                const STATE_ID destination = state->current_game_state->next_state(NULL); //FIXME ad params to call

                // updates gamestes
                change_game_state(appstate, destination);
            }

            break;
    }
    return SDL_APP_CONTINUE;
}

/* runs at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    app_state *state = (app_state *) appstate;
    /* SDL will clean up the window/renderer for us. */

    //TODO end whole program when a single thread errors out

    destroy_frameQueue(state->render_queue);
    //SDL_DestroyAudioStream
}

/* //FIXME this happened on exit
Assertion (frame->private_ref && frame->private_ref->size == sizeof(FrameDecodeData)) || !(avctx->codec->capabilities &
(1 << 1)) failed at D:/code/ffmpeg/src/libavcodec/decode.c:705

FIXME I also got an error once when exiting out on an audio only chunk
Process finished with exit code -1073741819 (0xC0000005)

scaling issue when not rendereing a new frame when going from fullscreeen to windowed

sometimes starts blank or somshit onf first startup:
ERROR: waiting for video queue to empty timed out

Process finished with exit code -1073741510 (0xC000013A: interrupted by Ctrl+C)
*/