#ifndef __PacketQueue_C__
#define __PacketQueue_C__
#include "PacketQueue.h"


int PacketQueue::put_packet(AVPacket *pkt) {
	if (abort_request) {
		return -1;
	}
	int ret;
	std::unique_lock < std::mutex > lock(mutex);
	while (true) {
		if (queue.size() < MAX_SIZE) {
			//logd("queue.size()=%d < MAX_SIZE", queue.size());
			AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
			if (packet == NULL) {
				logd("PacketQueue::put_packet: av_malloc() error");
				return -1;
			}
			//logd("main_play.cpp:53, av_copy_packet(packet, pkt) in");
			ret = av_copy_packet(packet, pkt);
			//logd("main_play.cpp:55, av_copy_packet(packet, pkt) out");
			if (ret != 0) {
				logd("PacketQueue::put_packet: av_copy_packet() error");
				return -1;
			}
			queue.push(*packet);
			//logd("pushed a packet to queue, queue.size()=%d",queue.size());
			duration += packet->duration;
			cond.notify_one();
			break;
		}
		else {
			//logd("queue.size()=%d >= MAX_SIZE", queue.size());
			full.wait(lock);
		}
	}
	return 0;
}
int PacketQueue::get_packet(AVPacket *pkt) {
	std::unique_lock < std::mutex > lock(mutex);
	for (;;) {
		if (abort_request) {
			return -1;
		}
		if (queue.size() > 0) {
			AVPacket tmp = queue.front();
			//logd("main_play.cpp:78, av_copy_packet");
			av_copy_packet(pkt, &tmp);
			//logd("main_play.cpp:80, av_copy_packet success one");
			duration -= tmp.duration;
			queue.pop();
			av_packet_unref(&tmp);
			av_free_packet(&tmp);
			full.notify_one();
			return 0;
		}
		else {
			logd("PacketQueue::get_packet: PacketQueue is empty");
			cond.wait(lock);
		}
	}
}
int PacketQueue::put_nullpacket() {
	AVPacket *pkt = new AVPacket();
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	put_packet(pkt);
	return 0;
}
void PacketQueue::set_abort(int abort) {
	abort_request = abort;
}
int PacketQueue::get_abort() {
	return abort_request;
}
int PacketQueue::get_serial() { //好像没什么用
	return serial;
}
void PacketQueue::flush() {
	std::unique_lock < std::mutex > lock(mutex);
	while (queue.size() > 0) {
		AVPacket tmp = queue.front();
		queue.pop();
		av_packet_unref(&tmp);
		av_free_packet(&tmp); //官网现在写的是void av_packet_free(AVPacket **pkt)
	}
	duration = 0;
	full.notify_one(); //为什么只唤醒一个
}
size_t PacketQueue::get_queue_size() {
	return queue.size();
}

#endif
