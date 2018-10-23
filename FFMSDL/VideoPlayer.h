#ifndef __VideoPlayer_H__
#define __VideoPlayer_H__
#include "Player.h"

int show_frame_sdl(void *player);

extern SDL_Window *g_SDLWindow;
extern SDL_Renderer *g_SDLRenderer;
extern SDL_Texture *g_SDLTexture;
extern int g_SDLWindowWidth; 
extern int g_SDLWindowHeight;
extern void update();
extern SDL_Texture *g_TexProgress;
extern SDL_Rect g_RectProgress;

extern Player *g_Player;
extern bool g_quit;

#endif // !__VideoPlayer_H__
