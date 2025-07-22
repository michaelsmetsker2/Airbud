/**
 * @file playback.c
 *
 * Handles decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
*/

#include "../include/playback.h"
#include "../include/common.h"

// ffmpeg libraries
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>

Sint32 TIMEOUT_DELAY_MS = 125;

int AVPlayback(void *data) {
    const PlaybackArgs *args = (PlaybackArgs *) data;

    AVFormatContext *AVContext = NULL;

    // opens the file (only looks at header)
    if (avformat_open_input(&AVContext, args->filename, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        return false;
    }
    // finds the stream info
    if (avformat_find_stream_info(AVContext, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        return false;
    }

    // find the index of the best video stream and decoded
    const AVCodec *pVideoDecoder = NULL;
    const int VIDEO_STREAM_INDEX = av_find_best_stream(AVContext, AVMEDIA_TYPE_VIDEO, -1, -1, &pVideoDecoder, 0);
    if (VIDEO_STREAM_INDEX < 0 || !pVideoDecoder) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best video stream or decoder %d\n",
                     VIDEO_STREAM_INDEX);
        return false;
    }

    const AVStream *pStream = AVContext->streams[VIDEO_STREAM_INDEX];
    const AVCodecParameters *pCodecPar = pStream->codecpar;

    // Allocates memory for AVCodecContext
    AVCodecContext *pVideoCodecCtx = avcodec_alloc_context3(pVideoDecoder);
    if (!pVideoCodecCtx) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        return false;
    }
    // Fills it with values
    if (avcodec_parameters_to_context(pVideoCodecCtx, pCodecPar) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
        return false;
    }
    // Opens decoder
    if (avcodec_open2(pVideoCodecCtx, pVideoDecoder, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open codec\n");
        return false;
    }

    //alloc packet and frame
    AVPacket *videoPacket = av_packet_alloc();
    AVFrame *videoFrame = av_frame_alloc();
    if (!videoPacket || !videoFrame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate packet or frame\n");
        return SDL_APP_FAILURE;
    }

    //while there is unparsed data left in the file
    while (!args->shouldExit && av_read_frame(AVContext, videoPacket) >= 0) {
        //make sure it belongs to the right stream TODO make sure this check is needed / see if its used for audio as well in the loop
        if (videoPacket->stream_index == VIDEO_STREAM_INDEX) {
            if (avcodec_send_packet(pVideoCodecCtx, videoPacket) == 0) {
                //a full frame is ready
                while (!args->shouldExit && avcodec_receive_frame(pVideoCodecCtx, videoFrame) == 0) {
                    //clone the frame
                    AVFrame *clonedVideoFrame = av_frame_clone(videoFrame);
                    if (!clonedVideoFrame) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't clone frame\n");
                        break; //can't just return as cleanup is needed
                    }

                    //wrap fram in struct
                    Frame *frameWrapper = malloc(sizeof(Frame));
                    if (!frameWrapper) {
                        av_frame_free(&clonedVideoFrame);
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate frameWrapper\n");
                        break;
                    }

                    frameWrapper->videoFrame = clonedVideoFrame;
                    frameWrapper->audioFrame = NULL; //TODO audio implementation
                    //TODO button implementation

                    //TODO should definitely make this while loop a function
                    //loop mutex is available and the queue is not full
                    while (!args->shouldExit) {
                        SDL_LockMutex(args->queue->mutex); // waits until mutex is unlocked

                        if (!args->shouldExit && args->queue->size != FRAME_QUEUE_CAPACITY) {
                            //queue is not at capacity
                            if (!enqueueFrame(args->queue, frameWrapper)) {
                                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                                destroyFrame(frameWrapper);
                                break;
                            }
                        } else {
                            //queue is at capacity
                            if (SDL_WaitConditionTimeout(args->queue->notFull, args->queue->mutex, TIMEOUT_DELAY_MS)) {
                                if (!enqueueFrame(args->queue, frameWrapper)) {
                                    // successful queue
                                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't enqueue frame\n");
                                    destroyFrame(frameWrapper);
                                    break;
                                }
                            } // timed out
                        }
                        SDL_UnlockMutex(args->queue->mutex);
                    } //queue loop, please make me into a function
                } //full frame decoded
            } // if packet has info to decode
        } // check index
        av_packet_unref(videoPacket);
    } // Read Packet loop

    // Flush decoder after file ends
    avcodec_send_packet(pVideoCodecCtx, NULL);
    while (avcodec_receive_frame(pVideoCodecCtx, videoFrame) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Flushed frame: %dx%d, pts=%lld\n", videoFrame->width,
                     videoFrame->height, videoFrame->pts);
        // Process flushed frames as well
    }

    av_frame_free(&videoFrame);
    av_packet_free(&videoPacket);

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
    }

    /*
    if (frame->pts != AV_NOPTS_VALUE) {
        double pts_seconds = frame->pts * av_q2d(stream->time_base);
        printf("Decoded frame: %dx%d, pts=%lld (%.3f sec)\n", frame->width, frame->height, frame->pts, pts_seconds);
    } else {
        printf("Decoded frame: %dx%d, pts=NOPTS\n", frame->width, frame->height);
    }


    while (av_read_frame(AVContext, pPacket) >= 0) {

        if (pPacket->stream_index == VIDEO_STREAM_INDEX) {
            if (avcodec_send_packet(pVideoCodecCtx, pPacket) == 0) {
                while (avcodec_receive_frame(pVideoCodecCtx, pFrame) == 0) {

                    int64_t pts = pFrame->best_effort_timestamp;
                    //printf("Decoded frame: %dx%d, pts=%lld\n", frame->width, frame->height, pts);

                    SDL_Texture *texture = SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_IYUV,   // Equivalent to YUV420 planar
                    SDL_TEXTUREACCESS_STREAMING,
                    pVideoCodecCtx->width,
                    pVideoCodecCtx->height);

                    SDL_UpdateYUVTexture(texture,
                        NULL,               // entire texture
                        pFrame->data[0], pFrame->linesize[0],   // Y plane
                        pFrame->data[1], pFrame->linesize[1],   // U plane
                        pFrame->data[2], pFrame->linesize[2]);  // V plane

                    SDL_RenderClear(renderer);
                    SDL_RenderTexture(renderer, texture, NULL, NULL);  // whole texture to window
                    SDL_RenderPresent(renderer);

                    //SDL_Delay(16);
                    SDL_DestroyTexture(texture);
                }
            }
        }
        av_packet_unref(pPacket);
    }

    // Flush decoder after file ends
    avcodec_send_packet(pVideoCodecCtx, NULL);
    while (avcodec_receive_frame(pVideoCodecCtx, pFrame) == 0) {
        printf("Flushed frame: %dx%d, pts=%lld\n", pFrame->width, pFrame->height, pFrame->pts);
        // Process flushed frames as well
    }

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    */
}
