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
#include "../include/decode.h"

Sint32 TIMEOUT_DELAY_MS = 125;


int play_file(void *data) {
    const struct playback_args *args = (struct playback_args *) data;

    AVFormatContext *current_file = NULL;

    // opens the file (only looks at header)
    if (avformat_open_input(&current_file, args->filename, NULL, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open the file");
        *args->exit_flag = true;
    }
    // finds the stream info
    if (avformat_find_stream_info(current_file, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find stream info");
        *args->exit_flag = true;
    }

    // find the index of the best video stream and decoded //TODO p sure all these are the same
    const AVCodec *video_codec = NULL;
    const int video_stream_index = av_find_best_stream(current_file, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    if (video_stream_index < 0 || !video_codec) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't find best video stream or decoder %d\n",
                     video_stream_index);
        *args->exit_flag = true;
    }

    // Allocates memory for AVCodecContext //TODO same here
    AVCodecContext *video_codec_context = avcodec_alloc_context3(video_codec);
    if (!video_codec_context) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate codec context\n");
        *args->exit_flag = true;
    }

    // Fills it with values
    { //variable declarations are just for readability //TODO i don't like this though
        const AVStream *video_stream = current_file->streams[video_stream_index];
        const AVCodecParameters *video_codec_parameters = video_stream->codecpar;

        if (avcodec_parameters_to_context(video_codec_context, video_codec_parameters) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't copy codec parameters\n");
            *args->exit_flag = true;
        }
    }

    // Opens decoder
    if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't open codec\n");
        *args->exit_flag = true;
    }

    //alloc packet and frame
    AVPacket *packet = av_packet_alloc();
    AVFrame *video_frame = av_frame_alloc();
    if (!packet || !video_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate packet or frame\n");
        *args->exit_flag = true;
    }

    //av_dump_format(AVContext, 0, args->filename, 0); TODO debug shit remove or use

    //while there is unparsed data left in the file
    while (!(*args->exit_flag) && av_read_frame(current_file, packet) >= 0) {

        //if stream index is audio, run that shit


        //does packet belong to the video stream TODO make sure this check is needed
        if (packet->stream_index == video_stream_index) {

            decode_video(packet);




        }
        av_packet_unref(packet);
    } // Read Packet loop

    // Flush decoder after file ends
    avcodec_send_packet(video_codec_context, NULL);
    while (avcodec_receive_frame(video_codec_context, video_frame) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Flushed frame:");
        // Process flushed frames as well
    }

    av_frame_free(&video_frame);
    av_packet_free(&packet);


    //TODO set stop_decoder_thread to false again? or maybe at begining of file

    return true;
}