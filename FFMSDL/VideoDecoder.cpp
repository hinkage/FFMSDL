#ifndef __VideoDecoder_C__
#define __VideoDecoder_C__
#include "VideoDecoder.h"

int VideoDecoder::decode_one_packet() { //复写父类的虚函数
	int ret;

	do {
		if (pkt_queue.get_abort())
			return -1;
		if (!packet_pending || pkt_queue.get_serial() != pkt_serial) {
			//logd("line244:pkt_queue.get_queue_size()=%d",pkt_queue.get_queue_size());
			if (pkt_queue.get_packet(&pkt) < 0) { //pkt是内部成员变量
				return -1;
				if (pkt.data == NULL) { //c++初始化变量都是置0吗？不然怎么会是NULL？或者是由ffmpeg处理为0的？
					logd("VideoDecoder::decode_one_packet(): reach eof.\n"); //对安卓或许将不可见
					return -1;
				}
			}
			//logd("line252:pkt_queue.get_queue_size()=%d",pkt_queue.get_queue_size());
			//下面的send和receive是新版ffmpeg的，旧版没有
			//发进去的的是packet，取出来的是frame
			ret = avcodec_send_packet(avctx, &pkt);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				logd("VideoDecoder::decode_one_packet(): video avcodec_send_packet error %d.\n", ret);
				break;
			}
			//logd("line257:video avcodec_send_packet succeed");

			AVFrame *frame = av_frame_alloc(); //分配到堆
			ret = avcodec_receive_frame(avctx, frame);
			if (ret < 0 && ret != AVERROR_EOF) {//得到错误码-11，而这据官网说是合法的other negative values: legitimate decoding errors
				logd("VideoDecoder::decode_one_packet(): video avcodec_receive_frame error %d.\n", ret);
				break;
			}
			//double pts;           /* presentation timestamp for the frame */
			frame->pts = av_frame_get_best_effort_timestamp(frame);
			frame_queue.put_frame(frame);
			logd("VideoDecoder::decode_one_packet(): put one frame to VideoDecoder.frame_queue,framesize=%d", this->frame_queue.get_size());
			if (ret < 0) {
				packet_pending = 0; //已经没有更多的packet了
			}
			else {
				//wait for adding
			}
		}
	} while (ret != 0 && !finished); //虽然finished没有用到过，但是上面的break已经够用了
	return 0;
}

void VideoDecoder::start_decode() {
	while (true) {
		if (pkt_queue.get_abort())
			break; //abort是一个开关
		int got_picture; //抄的谁？
		if ((got_picture = decode_one_packet()) < 0) {
			logd("VideoDecoder::start_decode(): decode_one_packet() return <0");
			return;
		}
	}
}
//AVCodecContext *avctx;返回的正是这个结构体里面保存的宽高信息
int VideoDecoder::get_width() {
	if (!avctx)
		return 0;
	return avctx->width;
}

int VideoDecoder::get_height() {
	if (!avctx)
		return 0;
	return avctx->height;
}

#endif // !__VideoDecoder_C__
