#include "AudioPlayer.h"

Uint32 audio_len = 0;
Uint8 *audio_pos = NULL;
Uint8 *audio_chunk = NULL;

void callback_audio(void *udata, Uint8 *stream, int len) {
	SDL_memset(stream, 0, len);
	if (audio_len == 0) {
		return;
	}
	len = (len>audio_len ? audio_len : len);
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int play_audio_sdl(void *player) {
	Player *mPlayer = (Player*)player;
	logd("main.play_audio_sdl(): I am in main.play_audio_sdl()");
	mPlayer->wait_state(PlayerState::READY);//视频READY也会导致audio被READY，实则没有audio没有READY,所以出现了下面的空指针的错误

											//int out_sample_rate = 44100;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
	uint8_t *out_buffer;
	int nextSize;
	//调试到下面这行时，崩溃，ffmpeg的那个函数没有成功运行
	//VS直接就可以指出：引发了异常: 读取访问权限冲突。mPlayer->auddec.avctx 是 nullptr。
	int out_buffer_size = av_samples_get_buffer_size(NULL, mPlayer->auddec.avctx->channels, mPlayer->auddec.avctx->frame_size, out_sample_fmt, 1);
	//out_buffer = (uint8_t *)av_malloc(192000 * 2);//之前没有 改为
	out_buffer = (uint8_t*)av_malloc(out_buffer_size);

	SDL_AudioSpec wanted_spec;

	wanted_spec.freq = mPlayer->auddec.avctx->sample_rate;//不再用44100，然后视频播放的速度正常了，之前有些快
	wanted_spec.format = AUDIO_S16SYS;//这里不一样
	wanted_spec.channels = mPlayer->auddec.avctx->channels;//之前用的out_channels，没什么区别
	wanted_spec.silence = 0;
	wanted_spec.samples = mPlayer->auddec.avctx->frame_size;
	wanted_spec.callback = callback_audio;
	wanted_spec.userdata = mPlayer->auddec.avctx;
	logd("before SDL_OpenAudio: format is S16SYS=%d, freq=%d, channels=%d, samples=%d", wanted_spec.format == AUDIO_S16SYS, wanted_spec.freq, wanted_spec.channels, wanted_spec.samples);

	if (SDL_OpenAudio(&wanted_spec, NULL)<0) {//stereo n.立体声
		logd("main.play_audio_sdl(): SDL_OpenAudio() error");
		return 0;
	}
	logd("after SDL_OpenAudio: format is S16SYS=%d, freq=%d, channels=%d, samples=%d", wanted_spec.format == AUDIO_S16SYS, wanted_spec.freq, wanted_spec.channels, wanted_spec.samples);

	bool start = false;
	while (mPlayer->get_aud_buffer(nextSize, out_buffer)) {
		while (audio_len>0) {
			SDL_Delay(1);
		}

		audio_chunk = (Uint8 *)out_buffer;//之前这三行放到while外面去了
		audio_len = out_buffer_size;//out_buffer_size; //mPlayer->auddec.audio_len;
		audio_pos = audio_chunk;
		logd("main.play_audio_sdl(): audio_len = %d, audio_pos = %lld", audio_len, audio_pos);//%lld用于输出64位数，否则会输出负数

		if (start == false) {
			logd("main.play_audio_sdl() --> SDL_PauseAudio(0)");
			SDL_PauseAudio(0);//这一行开始播放时，都还没有get_aud_buffer
			start = true;
		}
	}

	return 0;
}