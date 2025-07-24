/**
 * @file playback.c
 *
 * Handles decoding and adding frames to the queue.
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <libavformat/avformat.h>

#include "../include/playback.h"
#include "../include/common.h"
#include "../include/decode.h"

//av_dump_format(AVContext, 0, args->filename, 0); TODO useful debug

int play_file(void *data) {
    const struct playback_args *args = (struct playback_args *) data;

    //making sure these are all defined by cleanup
    AVFormatContext *current_file = NULL;
    AVCodecContext *video_codec_context = NULL;
    AVPacket *packet = NULL;
    AVFrame *video_frame = NULL;

    // opens the file (only looks at header)
    if (avformat_open_input(&current_file, args->filename, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        goto fail;
    }
    // finds the stream info
    if (avformat_find_stream_info(current_file, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        goto fail;
    }

    // find the index of the best video stream and decoded //TODO p sure all these are the same
    const AVCodec *video_codec = NULL;
    const int video_stream_index = av_find_best_stream(current_file, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (video_stream_index < 0 || !video_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best video stream or decoder %d\n",
                     video_stream_index);
        goto fail;
    }

    // Allocates memory for AVCodecContext //TODO same here
    video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        goto fail;
    }

    { //variable declaration is just for readability
        const AVCodecParameters *video_codec_parameters = current_file->streams[video_stream_index]->codecpar;
        if (avcodec_parameters_to_context(video_codec_context, video_codec_parameters) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
            goto fail;
        }
    }

    // Opens decoder
    if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open codec\n");
        goto fail;
    }

    //alloc packet and video_frame
    packet = av_packet_alloc();
    video_frame = av_frame_alloc();
    if (!packet || !video_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate packet or frame\n");
        goto fail;
    }

    //while there is unparsed data left in the file
    while (!(*args->exit_flag) && av_read_frame(current_file, packet) >= 0) {

        //if stream index is audio, run that shit


        //does packet belong to the video stream TODO make sure this check is needed
        if (!(*args->exit_flag) && packet->stream_index == video_stream_index) {

            decode_video(video_codec_context, packet, video_frame, args->queue, args->exit_flag);

        }
        av_packet_unref(packet);
    }

    // Cleanup
    av_frame_free(&video_frame);
    av_packet_free(&packet);
    avcodec_free_context(&video_codec_context);
    avformat_close_input(&current_file);
    return true;

    fail:
        *args->exit_flag = true;
    av_frame_free(&video_frame);
    av_packet_free(&packet);
    avcodec_free_context(&video_codec_context);
    avformat_close_input(&current_file);
    return false;
}