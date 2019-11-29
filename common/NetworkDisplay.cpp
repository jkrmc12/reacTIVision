#include "NetworkDisplay.h"

extern "C"
{
#include <libavcodec/avcodec.h>

}

NetworkDisplay::NetworkDisplay(const char* endpoint) {

	avcodec_register_all();

	codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
	codecContext_ = avcodec_alloc_context3(codec_);
	frame_ = av_frame_alloc();

	avcodec_open2(codecContext_, codec_, NULL);
}

NetworkDisplay::~NetworkDisplay() {
	avcodec_close(codecContext_);
	av_frame_free(&frame_);
	avcodec_free_context(&codecContext_);
}

bool NetworkDisplay::init(int w, int h, int sf, int df) {
	FrameProcessor::init(w, h, sf, df);

	codecContext_->width = frame_->width = width;
	codecContext_->height = frame_->height = height;
	codecContext_->pix_fmt = AV_PIX_FMT_YUV422P;
	frame_->format = (int)AV_PIX_FMT_YUV422P;

	if (avcodec_open2(codecContext_, codec_, NULL) > 0) {
		return false;
	}
	if (av_frame_get_buffer(frame_, 32)) {
		return false;
	}

	return true;
}

void NetworkDisplay::postProcess(const unsigned char* display) {

	av_frame_make_writable(frame_);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			frame_->data[0][y * frame_->linesize[0] + x] = display[y * frame_->linesize[0] + x];
		}
	}



	AVPacket packet;
	int packet_ptr = 0;

	av_init_packet(&packet);
	avcodec_encode_video2(codecContext_, &packet, frame_, &packet_ptr);

	av_free_packet(&packet);
}

void encode(AVFrame* frame, AVPacket* pkt, uint8_t* output)
{
	//int ret;
	/* send the frame to the encoder */
	
	//ret = avcodec_send_frame(enc_ctx, frame);
	//if (ret < 0) {
	//	fprintf(stderr, "error sending a frame for encoding\n");
	//	exit(1);
	//}
	//while (ret >= 0) {
	//	ret = avcodec_receive_packet(enc_ctx, pkt);
	//	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	//		return;
	//	else if (ret < 0) {
	//		fprintf(stderr, "error during encoding\n");
	//		exit(1);
	//	}
	//	printf("encoded frame %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
	//	fwrite(pkt->data, 1, pkt->size, outfile);
	//	av_packet_unref(pkt);
	//}
}