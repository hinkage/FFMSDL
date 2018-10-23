#ifndef __FrameQueue_H__
#define __FrameQueue_H__
#include "Frame.h"

class FrameQueue {
public:
	void put_frame(AVFrame *frame);

	std::shared_ptr<Frame> get_frame();

	size_t get_size();

	int64_t frame_queue_last_pos();
private:
	std::queue<std::shared_ptr<Frame>> queue;
	std::mutex mutex;
	std::condition_variable empty;//miss 'a'
	std::condition_variable full;
	const size_t MAX_SIZE = 16;
};

#endif // !__FrameQueue_H__

