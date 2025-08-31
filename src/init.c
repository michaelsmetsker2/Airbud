/**
 * @file init.c
 *
 * functions for initialization and populating critical structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <init.h>
#include <read_file.h>
#include <render.h>
#include <game_states.h>

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 480

//audio packet format stream
static const SDL_AudioSpec format = {
    .freq = 48000,
    .format = SDL_AUDIO_S16LE,
    .channels = 2, //stereo
};

app_state *initialize() {
    SDL_SetAppMetadata("airbud", "1.0", "com.airbud.renderer");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    // Creates empty app_state struct
    app_state *appstate = malloc(sizeof(app_state));
    if (!appstate) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return NULL;
    }
    // Creates window and renderer and adds them to app_state
    if (!SDL_CreateWindowAndRenderer("airbud/renderer", SCREEN_WIDTH, SCREEN_HEIGHT,
        0, &appstate->window, &appstate->renderer))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create window/renderer\n");
        return NULL;
    }

    // frame_queue for the app
    appstate->render_queue = create_frame_queue();
    if (!appstate->render_queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate video frame_queue\n");
        return NULL;
    }

    //creates reused texture for rendering
    appstate->base_texture = SDL_CreateTexture(appstate->renderer,
        SDL_PIXELFORMAT_IYUV,   // Equivalent to YUV420 planar
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);

    // sets total_audio_samples to 0
    SDL_SetAtomicU32(&appstate->total_audio_samples, 0);
    // set initial gamestate to the main menu

    appstate->current_game_state = &GAME_STATES[MAIN_MENU_1];
    appstate->renderer_mutex = SDL_CreateMutex();
    if (!appstate->renderer_mutex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create mutex\n");
        return NULL;
    }

    // sets id of decoding ended event
    appstate->decoding_ended_event = SDL_RegisterEvents(1);
    if (!appstate->decoding_ended_event) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't register decoder end event\n");
        return NULL;
    }

    return appstate;
}

bool  start_threads(app_state *appstate) {

    //starts sdl audio stream
    appstate->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, NULL, NULL);
    if (!appstate->audio_stream) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create audio stream\n");
        return false;
    }
    SDL_ResumeAudioStreamDevice(appstate->audio_stream);

    //starts both threads
    if (!create_decoder_thread(appstate)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to initiazlize the decoder thread\n");
        return false;
    }

    if (!create_render_thread(appstate)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to initialize the render thread\n");
        return false;
    }

    return true;
}