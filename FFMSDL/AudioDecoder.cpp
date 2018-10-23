#ifndef __AudioDecoder_C__
#define __AudioDecoder_C__
#include "AudioDecoder.h"
int cnt = 0;
int AudioDecoder::decode_one_packet() {
	int ret;

	do {
		if (pkt_queue.get_abort())
			return -1;
		if (!packet_pending || pkt_queue.get_serial() != pkt_serial) {
			if (pkt_queue.get_packet(&pkt) < 0)
				return -1;
		}
		if (pkt.data == NULL) {
			logd("reach eof.\n");
			return -1;
		}
		//为什么用include *.h之后会出现这种错误？-- 因为if (!packet_pending || pkt_queue.get_serial() != pkt_serial)里面的值都没有被初始化为0
		//但是在ndk里面，.h文件里的申明的变量仍然是自动设为0或NULL的，这一点上，编译器处理得不一样，vc的编译器只在定义时默认给0值，所以在cpp文件里是默认的0值
		ret = avcodec_send_packet(avctx, &pkt);//用include *.cpp的时候没有出现这种错：0xC0000005: 读取位置 0xFFFFFFFFFFFFFFFF 时发生访问冲突。

		if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
			break;
		//对于音频，一个packet可能包含多个frame，所以要取完。之前的声音不连贯，可能就是因为没有取完
		//可是这样之后，没有声音了
		
		int got_frame = 0;
		cnt = 0;
		while (true) {
			AVFrame *frame = av_frame_alloc();
			ret = avcodec_receive_frame(avctx, frame);//Note that the function will always call *av_frame_unref(frame) before doing anything else.所以，这里出错了，frame作为循环外定义的变量，被清空了，而put进去的不是一个拷贝而只是一个指针
			logd("AudioDecoder::decode_one_packet(): avcodec_receive_frame(avctx, frame) return %d", ret);
			if (ret < 0 && ret != AVERROR_EOF) {
				av_frame_free(&frame);//it will call  av_frame_unref(*frame), so do not need unref here
				return 0;
			}
			//ret = avcodec_decode_audio4(this->avctx, frame,&got_frame, &this->pkt);
			//if (ret < 0) {
			//	logd("AudioDecoder::decode_one_packet: error in decoding packet");
			//	break;
			//}

			frame->pts = av_frame_get_best_effort_timestamp(frame);
			frame_queue.put_frame(frame);
			logd("AudioDecoder::decode_one_packet(): cnt=%d--------a audio frame put into the queue", cnt++);
		}
		//if (ret < 0 && ret != AVERROR_EOF)//不能在这里break
		//	break;

		/*ret = avcodec_receive_frame(avctx, frame);//这种，声音不完整
		if (ret < 0 && ret != AVERROR_EOF)
			break;
		frame->pts = av_frame_get_best_effort_timestamp(frame);
		frame_queue.put_frame(frame);*/

		if (ret < 0) {
			packet_pending = 0;
		}
		else {

		}
		
	} while (ret != 0 && !finished);

	return 0;
}

void AudioDecoder::start_decode() {
	AVFrame *frame = av_frame_alloc(); //分配到堆，回收在最后进程结束才进行，不过视频解码器没写这个分配内存
	while (true) {
		if (pkt_queue.get_abort())
			break;
		int got;
		if ((got = decode_one_packet()) < 0)
			return;
	}
}
int AudioDecoder::get_channels() {
	return avctx->channels;
}
int AudioDecoder::get_sample_rate() {
	return avctx->sample_rate;
}

#endif // !__AudioDecoder_C__
