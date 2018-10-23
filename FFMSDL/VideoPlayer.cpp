#include "VideoPlayer.h"

SDL_Rect RectVideo;

void update() {
	/*int xpos = 0, ypos = 0;
	SDL_GetWindowPosition(g_SDLWindow, &xpos, &ypos);
	logd("VideoPlayer: WindowPosition: xpos=%d, ypos=%d", xpos, ypos);
	int width = 0, height = 0;
	SDL_GetWindowSize(g_SDLWindow, &width, &height);
	logd("VideoPlayer: WindowSize: width=%d, height=%d", width, height);
	SDL_GetRendererOutputSize(g_SDLRenderer, &width, &height);
	logd("VideoPlayer: RendererOutputSize: width=%d, height=%d", width, height);*/

	SDL_Rect L_rect, R_rect;//坐标原点在左上角，坐标系是Texture的坐标系

	L_rect.x = 0; R_rect.x = g_Player->get_progress_bar() * g_SDLWindowWidth;
	L_rect.y = 0; R_rect.y = 0;
	L_rect.w = g_Player->get_progress_bar() * g_SDLWindowWidth; R_rect.w = g_SDLWindowWidth - g_Player->get_progress_bar() * g_SDLWindowWidth;
	L_rect.h = PROGRESSBAR_WIDTH; R_rect.h = PROGRESSBAR_WIDTH;

	SDL_SetRenderTarget(g_SDLRenderer, g_TexProgress);

	SDL_SetRenderDrawColor(g_SDLRenderer, 0xff, 0x00, 0x00, 0x00);//red
	SDL_RenderDrawRect(g_SDLRenderer, &L_rect);
	SDL_RenderFillRect(g_SDLRenderer, &L_rect);

	SDL_SetRenderDrawColor(g_SDLRenderer, 0x00, 0xff, 0, 0x00);//green
	SDL_RenderDrawRect(g_SDLRenderer, &R_rect);
	SDL_RenderFillRect(g_SDLRenderer, &R_rect);

	SDL_SetRenderTarget(g_SDLRenderer, NULL);//没有这个就是一片黑

	SDL_SetRenderDrawColor(g_SDLRenderer, 0x0, 0x0, 0x0, 0x0);//black 没有这个会闪现绿屏,andorid疯狂闪屏，windows问题不大，为什么？
	SDL_RenderCopy(g_SDLRenderer, g_TexProgress, NULL, &g_RectProgress);
	//SDL_RenderPresent(g_SDLRenderer);//画完再一起present，这样应该可以避免安卓闪屏
}

/* handle an event sent by the GUI */
static void event_loop() {
	SDL_Event event;
	while (g_SDLWindowHeight == 0) {
		//
	}
	//while (true) {
		if (true == g_quit) {
			exit(0);
		}

		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_KEYDOWN://768
			logd("event_loop: %d", event.type);
			break;
			//经过试验得出：SDL2的鼠标位置坐标也是基于屏幕坐标1280*720，坐标原点也是在左上角
		case SDL_MOUSEBUTTONDOWN://1025
								 //logd("event_loop: %d", event.type);
			if (event.button.button == SDL_BUTTON_LEFT) {
				if (event.button.y > g_SDLWindowHeight - PROGRESSBAR_WIDTH) {
					logd("main: button location: x=%d, y=%d", event.button.x, event.button.y);
					int64_t t = int64_t(((double)event.button.x / (double)g_SDLWindowWidth) * (double)g_Player->get_duration());
					g_Player->stream_seek(t);
				}
			}
			break;
		case SDL_MOUSEMOTION://1024
							 //logd("event_loop: %d", event.type);
			break;
		case SDL_WINDOWEVENT://512
							 //logd("event_loop: %d", event.type);
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				break;
			}
			break;
		case SDL_FINGERDOWN:
			if (event.tfinger.y > g_SDLWindowHeight - PROGRESSBAR_WIDTH) {
				logd("main: finger location: x=%d, y=%d", event.tfinger.x, event.tfinger.y);
				int64_t t = int64_t(((double)event.tfinger.x / (double)g_SDLWindowWidth) * (double)g_Player->get_duration());
				g_Player->stream_seek(t);
			}
			break;
		case SDL_QUIT://256
			g_quit = true;
			logd("event_loop: %d", event.type);
			break;
		default:
			break;
		}
	//}
}

int show_frame_sdl(void *player) {
	if (SDL_Init(SDL_INIT_EVERYTHING)) {//missed
		logd("VideoPlayer: Could not initialize SDL - %s", SDL_GetError());
		return 0;
	}

	Player *mPlayer = (Player*)player;
	logd("VideoPlayer.show_frame_sdl(): *************************into show_frame_sdl()");

	mPlayer->wait_state(PlayerState::READY);//missed 'er'
	logd("VideoPlayer.show_frame_sdl(): got READY");
	if (!mPlayer->has_video()) return 0;

	AVFrame *frameYUV = av_frame_alloc();
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, mPlayer->viddec.get_width(), mPlayer->viddec.get_height(), 1);
	logd("VideoPlayer.show_frame_sdl(): numBytes=%d", numBytes);
	uint8_t *vOutBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(frameYUV->data, frameYUV->linesize, vOutBuffer, AV_PIX_FMT_YUV420P, mPlayer->viddec.get_width(), mPlayer->viddec.get_height(), 1);

	mPlayer->wait_state(PlayerState::READY);
	//while (0 == g_SDLWindowWidth || 0 == g_SDLWindowHeight || NULL == g_TexProgress) {
		//等g_SDLWindowWidth的值正确之后 NULL == g_TexProgress是在安卓的SDL_RenderClear()报错之后加上的，有可能是因为空指针
	//}
	g_SDLWindow = SDL_CreateWindow("MediaPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		g_Player->viddec.get_width(), g_Player->viddec.get_height(), SDL_WINDOW_OPENGL);

#ifdef _WIN32
	SDL_GetWindowSize(g_SDLWindow, &g_SDLWindowWidth, &g_SDLWindowHeight);
	logd("VideoPlayer: GetWindowSize: w=%d, h=%d", g_SDLWindowWidth, g_SDLWindowHeight);
#else
	SDL_GetWindowSize(g_SDLWindow, &g_SDLWindowHeight, &g_SDLWindowWidth);
	logd("VideoPlayer: GetWindowSize: w=%d, h=%d", g_SDLWindowWidth, g_SDLWindowHeight);
#endif // _WIN32

	
	//g_SDLWindowWidth = g_Player->viddec.get_width();
	//g_SDLWindowHeight = g_Player->viddec.get_height();

	logd("main: Window: width=%d, height=%d", g_Player->viddec.get_width(), g_Player->viddec.get_height());//width=640, height=360，可见是基于屏幕坐标
																										   //WindowSize: width=640, height=360  RendererOutputSize: width=640, height=360 可见都是一样的基于屏幕坐标
	if (!g_SDLWindow) {
		logd("VideoPlayer.show_frame_sdl(): Could not create Window,%s", SDL_GetError());
		return 0;
	}
	g_SDLRenderer = SDL_CreateRenderer(g_SDLWindow, -1, 0);
	logd("VideoPlayer.show_frame_sdl(): SDL_CreateRenderer() finished");
	//1 = 0001, 2 = 0010, 0001 | 0010 = 0011, 所以SDL_TEXTUREACCESS_TARGET | SDL_TEXTUREACCESS_STREAMING会出错，导致无法绘制Texture，又浪费时间了2018/3/16/16/35
	g_SDLTexture = SDL_CreateTexture(g_SDLRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		mPlayer->viddec.get_width(), mPlayer->viddec.get_height());
	g_TexProgress = SDL_CreateTexture(g_SDLRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
		g_SDLWindowWidth, PROGRESSBAR_WIDTH);
	//SDL_SetTextureAlphaMod(g_SDLTexture, 0);
	//SDL_SetTextureBlendMode(g_SDLTexture, SDL_BLENDMODE_MOD);

	g_RectProgress.x = 0; g_RectProgress.w = g_SDLWindowWidth;
	g_RectProgress.y = g_SDLWindowHeight - PROGRESSBAR_WIDTH; g_RectProgress.h = PROGRESSBAR_WIDTH;

	RectVideo.x = 0; RectVideo.w = g_SDLWindowWidth;
	RectVideo.y = 0; RectVideo.h = g_SDLWindowHeight - PROGRESSBAR_WIDTH;
	logd("VideoPlayer.show_frame_sdl(): next into a loop of get_img_frame()");
	while (mPlayer->get_img_frame(frameYUV)) {
		if (mPlayer->get_paused()) {
			mPlayer->wait_paused();
		}
		logd("VideoPlayer.show_frame_sdl(): next SDL_UpdateTexture(),framesize=%d", mPlayer->viddec.frame_queue.get_size());

		SDL_RenderClear(g_SDLRenderer);
		update();

		SDL_UpdateTexture(g_SDLTexture, NULL, frameYUV->data[0], frameYUV->linesize[0]);		
		SDL_RenderCopy(g_SDLRenderer, g_SDLTexture, NULL, &RectVideo);
		SDL_RenderPresent(g_SDLRenderer);

		event_loop();
	}

	return 0;
}