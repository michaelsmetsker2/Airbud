/**
 * @file init.c
 *
 * functions for initialization and populating critical structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <common.h>
#include <init.h>
#include <read_file.h>

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

    // Creates reused frame_queue for the app
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

    // sets audio_playback_time to 0
    SDL_SetAtomicU32(&appstate->audio_playback_time, 0);

    return appstate;
}

bool start_threads(app_state *appstate) {

    //starts sdl audio stream
    appstate->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, NULL, NULL);
    if (!appstate->audio_stream) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create audio stream\n");
        return NULL;
    }

    SDL_ResumeAudioStreamDevice(appstate->audio_stream);

    // Creates decoder thread exit flag and sets it to false
    SDL_SetAtomicInt(&appstate->stop_decoder_thread, 0);

    // Initialize decoder thread args
    struct decoder_thread_args *decoder_args = create_decoder_args(appstate, TEST_FILE_URL);
    if (!decoder_args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create playback args\n");
        return false;
    }
    //This starts the decoder thread off on the first file
    appstate->decoder_thread = SDL_CreateThread(play_file, "decoder", decoder_args);
    if (!appstate->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder thread\n");
        return false;
    }

    return true;
}