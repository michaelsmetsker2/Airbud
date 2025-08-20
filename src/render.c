/**
 * @file decode.h
 *
 * functions for rendering thread and syncing rendering to the audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <libavutil/frame.h>
#include <stdint.h>

#include <frame_queue.h>
#include <render.h>
#include <init.h>

#define TIMEOUT_DELAY_MS 125
#define NUM_CHANNELS 2 // stereo
#define BYTES_PER_SAMPLE 2 // 16 bit

#define SAMPLES_TO_MS  (1000.0 / 48000) //sample rate is 48000 / second
#define PTS_TO_MS      (1000.0 / 90000.0) // time base is 1 / 90000 * 1000 for ms

#define AUDEO_LATENCY_MS 200 // hardware latency, 200ms seems to be good, can be adjusted if needed
#define LAG_TOLERANCE_MS 20.0 // won't drop a frame in only a small amount behind

/**
 * @struct render_thread_args
 * @brief Struct containing nesesary information for the render thread
 * although these are just a subset of appstate, this allows for more encapsulization
 */
struct render_thread_args {
    SDL_AtomicInt *exit_flag;             /**< exit flag for safe quick exit */

    SDL_Window *window;                   /**< main window for the app */
    SDL_Renderer *renderer;               /**< main renderer for the app */
    SDL_Texture *texture;                 /**< reused texture to avoid repeate declarations and memory churn */

    frame_queue *queue;                   /**< queue of avframes to render */
    SDL_AtomicU32 *total_audio_samples;   /**< total amount of audio samples pushed to the audio queue, used for syncing */
    SDL_AudioStream *audio_stream;        /**< audio stream where audio packets are queued */

    const struct game_state **game_state; /**< pointer to the pointer to the current game state, not to be changed from this thread */
    SDL_Mutex *state_mutex;               /**< mutex to keep the main thread from changing teh game state mid render cycle */
};

bool create_render_thread(app_state *appstate) {

    SDL_SetAtomicInt(&appstate->stop_render_thread, 0);

    // creates and populates args
    struct render_thread_args *args = malloc(sizeof(struct render_thread_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for render thread\n");
        return false;
    }

    args->exit_flag = &appstate->stop_render_thread;
    args->renderer = appstate->renderer;
    args->window = appstate->window;
    args->texture = appstate->base_texture;
    args->queue = appstate->render_queue;
    args->total_audio_samples = &appstate->total_audio_samples;
    args->audio_stream = appstate->audio_stream;
    args->game_state = &appstate->current_game_state;
    args->state_mutex = appstate->state_mutex;

    //starts decoder thread
    appstate->decoder_thread = SDL_CreateThread(render_frames, "decoder", args);
    if (!appstate->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate render thread\n");
        return false;
    }

    return true;
}

/**
 * @brief main render loop
 * @param args all nesesary information in a render_thread_args struct
 * @return true on success, false otherwise
 */
static bool render_loop(const struct render_thread_args *args) {
    /* waits for queue mutex */
    SDL_LockMutex(args->queue->mutex);

    if (args->queue->size == 0) {
        if (!SDL_WaitConditionTimeout(args->queue->not_empty, args->queue->mutex, TIMEOUT_DELAY_MS)) {
            //timeout, it is not abnormal for no frames to be in the queue, does not set exit thread
            SDL_UnlockMutex(args->queue->mutex);
            return false;
        }
    }

    AVFrame *current_frame = dequeue_frame(args->queue);
    SDL_UnlockMutex(args->queue->mutex);
    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error dequeueing frame");
        return false;
    }

    // sync audio and video
    {
        const uint32_t queued_samples = SDL_GetAudioStreamQueued(args->audio_stream) / (NUM_CHANNELS * BYTES_PER_SAMPLE);
        const uint32_t played_audio_samples = SDL_GetAtomicU32(args->total_audio_samples) - queued_samples;

        // timestamps of current audio and video frames in ms
        const double audio_time_ms = played_audio_samples * SAMPLES_TO_MS + AUDEO_LATENCY_MS;
        const double video_time_ms = (double)current_frame->best_effort_timestamp * PTS_TO_MS;

        if (video_time_ms > audio_time_ms ) {
            // delay until audio catches up
            // TODO warning when the wait is an obscene amount of time
            const Uint32 delay = (uint32_t)(video_time_ms - audio_time_ms);
            if (delay > 100) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "big ol delay of %l" PRId32, delay); //FIXME
            }
            SDL_Delay((uint32_t)(video_time_ms - audio_time_ms));
        } else if (audio_time_ms - video_time_ms > LAG_TOLERANCE_MS) {
            SDL_Log("dropping frame");
            av_frame_free(&current_frame);
            return true;
        }

        //FIXME remove debugging lines
        printf("played: %f" "\n", audio_time_ms);
        printf("pts   : %f" "\n", video_time_ms);
    }

    // render the frame
    SDL_UpdateYUVTexture(args->texture, NULL,
        current_frame->data[0], current_frame->linesize[0],   // Y plane
        current_frame->data[1], current_frame->linesize[1],   // U plane
        current_frame->data[2], current_frame->linesize[2]);  // V plane

    SDL_RenderClear(args->renderer);
    SDL_RenderTexture(args->renderer, args->texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(args->renderer);

    SDL_FlushRenderer(args->renderer);
    av_frame_free(&current_frame);
    return true;
}

int render_frames(void *data) {
    const struct render_thread_args *args = (struct render_thread_args *) data;

    while (SDL_GetAtomicInt(args->exit_flag) >= 0) {

        SDL_LockMutex(args->state_mutex);
        render_loop(args);

        if (SDL_GetAtomicInt(args->exit_flag) > 0) {

        }

    }

    //TODO cleanup?

    return true;
}