#ifndef __AudioDecoder_H__
#define __AudioDecoder_H__
#include "Decoder.h"

class AudioDecoder : public Decoder { //音频流解码器，增加了信道数和采样率的两个相关成员函数
public:
	int decode_one_packet();

	void start_decode();
	int get_channels();
	int get_sample_rate();
};
#endif // !__AudioDecoder_H__
