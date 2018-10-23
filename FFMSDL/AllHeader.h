#ifndef __AllHeader_H__  
#define __AllHeader_H__
#include <string.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <memory>
#include <stdio.h>

#include "log.h"

extern "C"{
#ifdef __cplusplus
 #define __STDC_CONSTANT_MACROS
 #ifdef _STDINT_H
  #undef _STDINT_H
 #endif
 # include <stdint.h>
#endif
}
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libavcodec/avfft.h"
#include "SDL.h"
}

enum class PlayerState {
	UNKNOWN, INIT, BUFFERING, READY, PLAYING,
};

#ifdef _WIN32
#define PROGRESSBAR_WIDTH 10
#else
#define PROGRESSBAR_WIDTH 50
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif  // _WIND32

// 定义64位整形
#if defined(_WIN32) && !defined(CYGWIN)
typedef __int64 int64_t;
#else
typedef long long int64_t;
#endif  // _WIN32

int64_t GetSysTimeMicros();
 
#endif
