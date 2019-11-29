#pragma once

#include "FrameProcessor.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}


class NetworkDisplay : public FrameProcessor
{
public:
	NetworkDisplay(const char* endpoint);
	~NetworkDisplay();

	bool init(int w, int h, int sf, int df);
	void process(unsigned char* src, unsigned char* dest) {}
	void postProcess(const unsigned char* display);

private:

	AVCodec* codec_;
	AVCodecContext* codecContext_;
	AVFrame* frame_;
};

