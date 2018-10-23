#ifndef __Decoder_H__
#define __Decoder_H__
#include "FrameQueue.h"
#include "PacketQueue.h"

class Decoder { //解码器的基类
public:
	virtual int decode_one_packet() = 0; //交给子类自己去写实现
	virtual void start_decode() = 0;
	void init(AVCodecContext *ctx);

	void start_decode_thread();

	PacketQueue pkt_queue;
	FrameQueue frame_queue;
	AVCodecContext *avctx = NULL;//如果不先确定为NULL，有错误：mPlayer->auddec.**avctx** 是 0xFFFFFFFFFFFFFE1B。以前包含cpp时，它自动设为NULL，现在用头文件，它就不是自动NULL了，不知道为什么 2018/3/9
protected:
	//子类的子类无法继承
	AVPacket pkt;
	AVPacket pkt_tmp;
	int pkt_serial;
	int finished;
	int packet_pending=0;
	std::condition_variable empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;//AVRational 有理数（分子和分母对）
	int64_t next_pts;
	AVRational next_pts_tb;
};
#endif // !__Decoder_H__
