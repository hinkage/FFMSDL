#ifndef __AudioPlayer_H__
#define __AudioPlayer_H__
#include "Player.h"

//SDL是用C写的，其接口函数只能申明在类外
int play_audio_sdl(void *player);

#endif // !__AudioPlayer_H__
