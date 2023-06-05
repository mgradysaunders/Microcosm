#include "Microcosm/Video"
#include <iostream>

#define EXIT_FAILURE_UNLESS(Cond) ExitFailureUnless(Cond, #Cond)

static void ExitFailureUnless(bool condition, const char *expression) noexcept {
  if (!condition) {
    std::cerr << "FFMPEG Error: !(" << expression << ")\n";
    std::exit(EXIT_FAILURE);
  }
}

namespace mi {

void Video::open(const std::string &filename, int width, int height, const Params &params) {
  avformat_alloc_output_context2(&mFormatContext, nullptr, nullptr, filename.c_str());
  EXIT_FAILURE_UNLESS(mCodec = avcodec_find_encoder(AV_CODEC_ID_H264));
  EXIT_FAILURE_UNLESS(mStream = avformat_new_stream(mFormatContext, nullptr));
  mStream->id = mFormatContext->nb_streams - 1;
  EXIT_FAILURE_UNLESS(mCodecContext = avcodec_alloc_context3(mCodec));

  mCodecContext->codec_id = mFormatContext->oformat->video_codec;
  mCodecContext->width = width;
  mCodecContext->height = height;
  mStream->time_base = av_d2q(1.0 / params.frameRate, 120);
  mCodecContext->time_base = mStream->time_base;
  mCodecContext->pix_fmt = params.targetFormat;
  mCodecContext->gop_size = 12;
  mCodecContext->max_b_frames = 2;
  if (mFormatContext->oformat->flags & AVFMT_GLOBALHEADER) mCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  EXIT_FAILURE_UNLESS(av_opt_set(mCodecContext->priv_data, "preset", "slow", 0) == 0);
  EXIT_FAILURE_UNLESS(av_opt_set_int(mCodecContext->priv_data, "crf", params.constantRateFactor, 0) == 0);
  EXIT_FAILURE_UNLESS(avcodec_open2(mCodecContext, mCodec, nullptr) == 0);
  EXIT_FAILURE_UNLESS(mFrame = av_frame_alloc());
  mFrame->format = mCodecContext->pix_fmt;
  mFrame->width = mCodecContext->width;
  mFrame->height = mCodecContext->height;
  EXIT_FAILURE_UNLESS(av_frame_get_buffer(mFrame, 32) >= 0);
  EXIT_FAILURE_UNLESS(avcodec_parameters_from_context(mStream->codecpar, mCodecContext) >= 0);
  mSwsContext = sws_getContext(
    mCodecContext->width, mCodecContext->height, params.sourceFormat, // src
    mCodecContext->width, mCodecContext->height, params.targetFormat, // dst
    SWS_BICUBIC, nullptr, nullptr, nullptr);
  EXIT_FAILURE_UNLESS(mSwsContext);
  av_dump_format(mFormatContext, 0, filename.c_str(), 1);
  EXIT_FAILURE_UNLESS(avio_open(&mFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) == 0);
  EXIT_FAILURE_UNLESS(avformat_write_header(mFormatContext, nullptr) >= 0);
  mFrameIndex = 0;
}

void Video::write(const unsigned char *frame, int lineSize) {
  EXIT_FAILURE_UNLESS(av_frame_make_writable(mFrame) >= 0);
  if (!lineSize) lineSize = 4 * mCodecContext->width;
  frame += lineSize * (mCodecContext->height - 1), lineSize = -lineSize; // Flip vertically.
  sws_scale(
    mSwsContext, &frame, &lineSize, 0, mCodecContext->height, // src
    mFrame->data, mFrame->linesize);                          // dst
  mFrame->pts = mFrameIndex++;
  EXIT_FAILURE_UNLESS(avcodec_send_frame(mCodecContext, mFrame) >= 0);
  flush();
}

void Video::flush() {
  do {
    AVPacket packet{};
    int result = avcodec_receive_packet(mCodecContext, &packet);
    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) break;
    EXIT_FAILURE_UNLESS(result >= 0);
    av_packet_rescale_ts(&packet, mCodecContext->time_base, mStream->time_base);
    packet.stream_index = mStream->index;
    EXIT_FAILURE_UNLESS(av_interleaved_write_frame(mFormatContext, &packet) >= 0);
    av_packet_unref(&packet);
  } while (true);
}

void Video::close() {
  avcodec_send_frame(mCodecContext, nullptr);
  flush();
  av_write_trailer(mFormatContext);
  EXIT_FAILURE_UNLESS(avio_close(mFormatContext->pb) == 0);
  if (mSwsContext) sws_freeContext(mSwsContext);
  if (mFrame) av_frame_free(&mFrame);
  if (mCodecContext) avcodec_free_context(&mCodecContext);
  if (mCodecContext) avcodec_close(mCodecContext);
  if (mFormatContext) avformat_free_context(mFormatContext);
}

} // namespace mi
