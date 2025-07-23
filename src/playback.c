/**
 * @file playback.c
 *
 * Handles decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>

#include "../include/playback.h"
#include "../include/common.h"

Sint32 TIMEOUT_DELAY_MS = 125;

int play_file(void *data) {
    const struct playback_args *args = (struct playback_args *) data;

    AVFormatContext *file_context = NULL;

    // opens the file (only looks at header)
    if (avformat_open_input(&file_context, args->filename, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        return -1;
    }
    // finds the stream info
    if (avformat_find_stream_info(file_context, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        return -1;
    }

    // find the index of the best video stream and decoded //TODO p sure all these are the same
    const AVCodec *video_codec = NULL;
    const int video_stream_index = av_find_best_stream(file_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (video_stream_index < 0 || !video_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best video stream or decoder %d\n",
                     video_stream_index);
        return -1;
    }

    // Allocates memory for AVCodecContext
    AVCodecContext *video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return -1;
    }

    // Fills it with values
    { //variable declarations are just for readability //TODO i don't like this though
        const AVStream *video_stream = file_context->streams[video_stream_index];
        const AVCodecParameters *video_codec_parameters = video_stream->codecpar;

        if (avcodec_parameters_to_context(video_codec_context, video_codec_parameters) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
            return -1;
        }
    }

    // Opens decoder
    if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open codec\n");
        return -1;
    }

    //alloc packet and frame
    AVPacket *video_packet = av_packet_alloc();
    AVFrame *video_frame = av_frame_alloc();
    if (!video_packet || !video_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate packet or frame\n");
        return SDL_APP_FAILURE;
    }

    //av_dump_format(AVContext, 0, args->filename, 0); TODO debug shit remove or use

    //while there is unparsed data left in the file
    while (!(*args->exit_flag) && av_read_frame(file_context, video_packet) >= 0) {
        //make sure it belongs to the right stream TODO make sure this check is needed / see if its used for audio as well in the loop
        if (video_packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(video_codec_context, video_packet) == 0) {
                //a full frame is ready
                while (!(*args->exit_flag) && avcodec_receive_frame(video_codec_context, video_frame) == 0) {

                    AVFrame *cloned_video_frame = av_frame_clone(video_frame); //clone the frame
                    if (!cloned_video_frame) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame\n");
                        break; //can't just return as cleanup is needed
                    }

                    //wrap decoded video frame in frame struct
                    struct frame *frameWrapper = malloc(sizeof(struct frame));
                    if (!frameWrapper) {
                        av_frame_free(&cloned_video_frame);
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frameWrapper\n");
                        break;
                    }

                    frameWrapper->video_frame = cloned_video_frame;
                    frameWrapper->audio_frame = NULL; //TODO audio implementation
                    //TODO button implementation

                    //TODO should definitely make this while loop a function
                    //loop mutex is available and the queue is not full
                    while (!(*args->exit_flag)) {
                        SDL_LockMutex(args->queue->mutex); // waits until mutex is unlocked

                        if (!(*args->exit_flag) && args->queue->size != FRAME_QUEUE_CAPACITY) {
                            //queue is not at capacity
                            if (!enqueue_frame(args->queue, frameWrapper)) {
                                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                                destroy_frame(frameWrapper);
                                break;
                            }
                        } else {
                            //queue is at capacity
                            if (SDL_WaitConditionTimeout(args->queue->not_full, args->queue->mutex, TIMEOUT_DELAY_MS)) {
                                if (!enqueue_frame(args->queue, frameWrapper)) {
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                                    destroy_frame(frameWrapper);
                                    break;
                                }
                            } // timed out
                        }
                        SDL_UnlockMutex(args->queue->mutex);
                    }
                }
            }
        }
        av_packet_unref(video_packet);
    } // Read Packet loop

    // Flush decoder after file ends
    avcodec_send_packet(video_codec_context, NULL);
    while (avcodec_receive_frame(video_codec_context, video_frame) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Flushed frame: %dx%d, pts=%lld\n", video_frame->width,
                     video_frame->height, video_frame->pts);
        // Process flushed frames as well
    }

    av_frame_free(&video_frame);
    av_packet_free(&video_packet);


    //TODO set stop_decoder_thread to false again? or maybe at begining of file

    return true;


    /*
        // find the index of the best audio stream and decoder
    const AVCodec *audioDecoder = NULL;
    const int AUDIO_STREAM_INDEX = av_find_best_stream(AVContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioDecoder, 0);
    if (AUDIO_STREAM_INDEX < 0 || !audioDecoder) {
        printf("couldn't find best audio stream or decoder %d\n", AUDIO_STREAM_INDEX);
        return SDL_APP_FAILURE;
    }

    // Allocates memory for AVCodecContext
    AVStream *audio_stream = AVContext->streams[AUDIO_STREAM_INDEX];
    AVCodecParameters *audio_codecpar = audio_stream->codecpar;

    // Fills it with values
    AVCodecContext *audio_codec_ctx = avcodec_alloc_context3(audioDecoder);
    if (!audio_codec_ctx) {
        printf("couldn't allocate audio codec context\n");
        return SDL_APP_FAILURE;
    }
    // Copies codec parameters
    if (avcodec_parameters_to_context(audio_codec_ctx, audio_codecpar) < 0) {
        printf("couldn't copy audio codec parameters\n");
        return SDL_APP_FAILURE;
    }
    // Opens decoder
    if (avcodec_open2(audio_codec_ctx, audioDecoder, NULL) < 0) {
        printf("couldn't open audio codec\n");
        return SDL_APP_FAILURE;
    */
    }
