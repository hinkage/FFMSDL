#ifndef __Frame_H__
#define __Frame_H__
#include "AllHeader.h"

struct Frame {
	Frame(AVFrame *f):frame(f) {
		this->pts = f->pts;
	}
	AVFrame *frame;
	int serial;
	double pts=0; /* presentation timestamp for the frame */
	double duration=0; /* estimated duration of the frame */
	int64_t pos=0; /* byte position of the frame in the input file */
	int allocated;
	int width;
	int height;
	int format;
	AVRational sar;
	int uploaded;
};
#endif
