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
#include <playback.h>

app_state *initialize() {
    SDL_SetAppMetadata("airbud", "1.0", "com.airbud.renderer");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    // Creates empty app_state struct
    app_state *state = malloc(sizeof(app_state));
    if (!state) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return NULL;
    }
    // Creates window and renderer and adds them to state
    if (!SDL_CreateWindowAndRenderer("airbud/renderer", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return NULL;
    }

    const SDL_AudioSpec desired = {
        .freq = 48000,
        .format = SDL_AUDIO_S16LE,
        .channels = 2, //stereo
    };
    state->audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired);
    if (!state->audio_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    // Creates a video and audio frame_queue for the app
    state->video_queue = create_frame_queue(VIDEO_BUFFER_CAP);
    if (!state->video_queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate video frame_queue\n");
        return NULL;
    }
    state->audio_queue = create_frame_queue(AUDIO_BUFFER_CAP);
    if (!state->video_queue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame_queue\n");
        return NULL;
    }

    state->base_texture = SDL_CreateTexture(state->renderer,
    SDL_PIXELFORMAT_IYUV,   // Equivalent to YUV420 planar
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH,
    SCREEN_HEIGHT);

    // Creates decoder thread exit flag and sets it to false
    state->stop_decoder_thread = calloc(1, sizeof(bool));
    if (!state->stop_decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder exit flag\n");
        return NULL;
    }

    // TODO do i have a way to clean up args? also File name needs updatin
    //create playback args for decoder thread and populate it //TODO make an args constructor function
    struct playback_args *args = malloc(sizeof(struct playback_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for decoder thread\n");
        return NULL;
    }
    args->exit_flag = state->stop_decoder_thread;
    args->video_queue = state->video_queue;
    args->audio_queue = state->audio_queue;
    args->filename = TEST_FILE_URL;

    //This starts the decoder thread off on the first file
    state->decoder_thread = SDL_CreateThread(play_file, "decoder", args);
    if (!state->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate decoder thread\n");
        return NULL;
    }

    return state;
}

bool start_threads(app_state *state) {

}