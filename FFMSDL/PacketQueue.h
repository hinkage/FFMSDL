#ifndef __PacketQueue_H__
#define __PacketQueue_H__
#include "AllHeader.h"

class PacketQueue {
public:
	int put_packet(AVPacket *pkt);
	int get_packet(AVPacket *pkt);
	int put_nullpacket();
	void set_abort(int abort);
	int get_abort();
	int get_serial();
	void flush();
	size_t get_queue_size();
private:
	std::queue<AVPacket> queue;
	int64_t duration;
	int abort_request = 0;//if(-1)为真的，之前include *.cpp的时候为什么没有break而在解码？
	int serial=0;
	std::mutex mutex;
	std::condition_variable cond;
	std::condition_variable full;
	const size_t MAX_SIZE = 16;//改成和FrameQueue一样的大小
	//80个会有一个最初的较长时间卡顿，8个偶尔在开头卡顿一会，怀疑这是queue内部的问题，其实也可以不用queue
	//改成两个，后面运行不卡，第一次运行卡，反正莫名其妙不知为何，一会有问题，一会没问题
	//不用队列的话呢？暂时先这样吧
};
#endif // !__PacketQueue_H__

