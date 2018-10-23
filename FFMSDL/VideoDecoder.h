#ifndef __VideoDecoder_H__
#define __VideoDecoder_H__
#include "Decoder.h"

class VideoDecoder : public Decoder { //视频流解码器，增加了视频的宽度和高度相关的两个成员函数
public:
	int decode_one_packet();

	void start_decode();
	//AVCodecContext *avctx;返回的正是这个结构体里面保存的宽高信息
	int get_width();

	int get_height();
};
#endif
