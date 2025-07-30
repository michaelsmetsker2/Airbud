/**
 * @file init.c
 *
 * functions for initialization and populating critical structs
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <audio.h>
#include <SDL3/SDL.h>
#include <common.h>
#include <init.h>
#include <read_file.h>
#include <audio.h>

//audio packet format for
static const int FREQUENCY = 48000;
static const SDL_AudioFormat FORMAT = SDL_AUDIO_S16LE;
static const int CHANNELS = 2; //stereo

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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create window/renderer\n");
        return NULL;
    }

    //create audio stream
    const SDL_AudioSpec desired = {
        .freq = FREQUENCY,
        .format = FORMAT,
        .channels = CHANNELS, //stereo
    };
    //TODO potentialy use the sdl audio callback thing?
    appstate->audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, NULL, NULL);
    if (!appstate->audio_stream) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create audio stream\n");
        return NULL;
    }

    // Creates reused video and audio frame_queue for the app
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

    //creates reused texture for rendering
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
    appstate->stop_audio_thread = calloc(1, sizeof(bool));
    if (!appstate->stop_decoder_thread|| !appstate->stop_audio_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate thread exit flags\n");
        return false;
    }
    // Initialize decoder thread args
    struct decoder_thread_args *decoder_args = create_decoder_args(appstate, TEST_FILE_URL); //FIXME filename needs updatin'
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

    struct audio_thread_args *audio_args = malloc(sizeof(struct audio_thread_args));
    if (!audio_args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create audio thread args\n");
        return false;
    }
    //initializes audio_args members
    audio_args->stream = appstate->audio_stream;
    audio_args->exit_flag = appstate->stop_audio_thread;
    audio_args->queue = appstate->audio_queue;
    //start audio thread
    appstate->audio_thread = SDL_CreateThread(audio_playback, "audio", audio_args);
    if (!appstate->audio_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't create audio thread\n");
        return false;
    }

    return true;
}