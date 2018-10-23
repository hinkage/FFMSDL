#include "Player.h"
#include <Windows.h>
#include "AudioPlayer.h"
#include "VideoPlayer.h"

Player *g_Player = NULL;
bool g_quit = false;

SDL_Window *g_SDLWindow = NULL;
SDL_Renderer *g_SDLRenderer = NULL;
SDL_Texture *g_SDLTexture = NULL;
int g_SDLWindowWidth = 0, g_SDLWindowHeight = 0;
SDL_Texture *g_TexProgress = NULL;
SDL_Rect g_RectProgress;

//把SDL_Init和与之相关的CreateWindow等等一系列都放到那个视频播放子线程，在安卓上终于就可以正常运行了，但是安卓的分辨率又有相关问题，时间啊
extern "C"
int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow
) {
	logc("VideoPlayer");
	logd("main.WinMain(): I am in");
	g_Player = new Player();
	g_Player->init("sintel.mp4");//D:\\1111111\\Baywatch.2017.WEB-DL.x264-FGT\\bb.avi //sintel.mp4
	g_Player->change_state_to_play();

	std::thread audioThread(play_audio_sdl, (void*)g_Player);//在安卓上卡顿
	std::thread videoThread(show_frame_sdl, (void*)g_Player);

	//g_Player->wait_state(PlayerState::READY);
	//SDL_CreateWindow需要放在和event_loop同一个线程，否则检测不到键盘事件只能检测到窗口和鼠标移动，
	//即使SDL_Init(SDL_INIT_VIDEO)和event_loop在同一个线程，这一点和官网所称有所出入，并且在SDL_CreateWindow
	//没有放到main函数里时，窗口的那三个button都是无法点击的，不知道为什么这样坑，该结果由实验所得
	//基于显示屏像素而不是显卡，也就是基于1280*720
	//又浪费时间了，安卓不能用| SDL_WINDOW_RESIZABLE创建窗口
	//而且，安卓不能在主线程CreateWindow，又浪费时间了，关键是android不好调试

	//event_loop();
	//exit(0);//若是没有这一行，直接return 0，会报错，实验所得，猜测原因可能是用exit才进行了资源回收，才能结束所有新开的线程

	audioThread.join();//加入了event_loop后，就不再需要join来阻止主线程退出了	
	videoThread.join();
	//SDL_CreateThread(play_audio_sdl, NULL, (void*)mPlayer);//在安卓上卡顿
	//SDL_CreateThread(show_frame_sdl, NULL, (void*)mPlayer);

	return 0;
}
