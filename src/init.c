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
    // Creates window and renderer and adds them to state
    if (!SDL_CreateWindowAndRenderer("airbud/renderer", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &appstate->window, &appstate->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return NULL;
    }

    const SDL_AudioSpec desired = {
        .freq = 48000,
        .format = SDL_AUDIO_S16LE,
        .channels = 2, //stereo
    };

    //FIXME
    appstate->audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired);
    if (!appstate->audio_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    // Creates a video and audio frame_queue for the app
    appstate->video_queue = create_frame_queue(VIDEO_BUFFER_CAP);
    if (!appstate->video_queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate video frame_queue\n");
        return NULL;
    }
    appstate->audio_queue = create_frame_queue(AUDIO_BUFFER_CAP);
    if (!appstate->video_queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame_queue\n");
        return NULL;
    }

    appstate->base_texture = SDL_CreateTexture(appstate->renderer,
    SDL_PIXELFORMAT_IYUV,   // Equivalent to YUV420 planar
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH,
    SCREEN_HEIGHT);

    return appstate;
}

bool start_threads(app_state *appstate) {

    // Creates decoder thread exit flag and sets it to false
    appstate->stop_decoder_thread = calloc(1, sizeof(bool));
    if (!appstate->stop_decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder exit flag\n");
        return false;
    }

    //TODO filename needs updatin'
    struct decoder_thread_args *args = create_decoder_args(appstate, TEST_FILE_URL);
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to create playback args\n");
        return false;
    }

    //This starts the decoder thread off on the first file
    appstate->decoder_thread = SDL_CreateThread(play_file, "decoder", args);
    if (!appstate->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder thread\n");
        return false;
    }

    return true;
}