#ifndef __Decoder_C__
#define __Decoder_C__
#include "Decoder.h"

void Decoder::init(AVCodecContext *ctx) {
	avctx = ctx; //指针的值传递，指针本身深拷贝，但是指针指向的AVCodecContext只是浅拷贝，不过由于这个ctx分配在堆上，故而可以使用浅拷贝
}

void Decoder::start_decode_thread() {
	pkt_queue.set_abort(0); //置0才能使用queue
	std::thread t(&Decoder::start_decode, this); //这里不用SDL提供的线程函数；decode是虚函数，那么线程内容依据子类的实现
	t.detach(); //不想detach，这样的话他不能伴随主线程的退出而退出怎么办？
}

#endif // !__Decoder_C__
