#ifndef __FrameQueue_C__
#define __FrameQueue_C__
#include "FrameQueue.h"

void FrameQueue::put_frame(AVFrame *frame) {
	std::unique_lock < std::mutex > lock(mutex);
	while (true) {
		if (queue.size() < MAX_SIZE) {
			//这里放入队列的指针还没有具体指向的地址，放入的只是指针本身的地址，也可能是指向指针的指针的地址
			auto m_frame = std::make_shared < Frame >(frame);
			if (!queue.empty()) {
				auto last_frame = queue.back();
				last_frame->duration = frame->pts - last_frame->pts;
			}
			queue.push(m_frame);
			empty.notify_one();
			return;
		}
		else {
			logd("FrameQueue::put_frame(): FrameQueue is full");
			full.wait(lock);
		}
	}
}

std::shared_ptr<Frame> FrameQueue::get_frame() {
	std::unique_lock < std::mutex > lock(mutex);
	while (true) {
		if (queue.size() > 0) {
			//出队列的指针没必要手动回收内存，指针指向的Frame的内存由系统管理，c++吸收了java的优点
			auto tmp = queue.front();
			queue.pop();
			full.notify_one();
			return tmp;//missed return
		}
		else {
			logd("FrameQueue::get_frame(): FrameQueue is empty");
			empty.wait(lock);
		}
	}
}

size_t FrameQueue::get_size() {
	return queue.size();
}

int64_t FrameQueue::frame_queue_last_pos() {
	auto frame = queue.back(); //队列最后的那个元素
	return frame->frame->pkt_pos;//AVFrame *frame;的pkt_pos 表示的是？
}

#endif // !__FrameQueue_C__
