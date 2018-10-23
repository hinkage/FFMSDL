#ifndef __Player_C__
#define __Player_C__
#include "Player.h"


Player::Player() = default;

void Player::set_cur_time_video(double t) {
	this->cur_time_video = t;
}

double Player::get_cur_time_video() {
	return this->cur_time_video;
}

void Player::set_cur_time_audio(double t) {
	this->cur_time_audio = t;
}

double Player::get_cur_time_audio() {
	return this->cur_time_audio;
}

void Player::set_file_path(const std::string input_filename) {
	if (input_filename.empty()) {
		logd("Player.set_file_path: input_filename is NULL");
		return;
	}
	logd("Player.set_file_path: Player init with file %s.", input_filename.c_str());
	this->file_path = av_strdup(input_filename.c_str());//duplicate 重复，复制
}

void Player::init(const std::string input_filename) {//--> read()
	if (input_filename.empty()) {
		logd("Player.set_file_path: An input file must be specified");
		return;
	}
	logd("Player.set_file_path: player init with file %s.", input_filename.c_str());
	file_path = av_strdup(input_filename.c_str());
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_register_all();
	avformat_network_init();
	logd("Player.set_file_path() --> std::thread read_thread(&Player::read, this)");
	std::thread read_thread(&Player::read, this);
	read_thread.detach();//宁愿在main里添加while，否则windows进程不能正常退出。但是如果这里不detach，程序会崩溃
}

void Player::start_read_thread() {
	av_register_all();
	avformat_network_init();
	std::thread read_thread(&Player::read, this);
	read_thread.detach();//不能用join，join会导致主线程陷在死循环里
}

bool Player::has_video() {
	if (format_ctx) {
		return video_stream_index>= 0;
	}
	return false;
}

bool Player::get_img_frame(AVFrame *frame) {
	logd("Player.get_img_frame: I am in get_img_frame()");
	if (frame == nullptr) {
		logd("Player.get_img_frame: frame is null");
		return false;
	}
	auto av_frame = viddec.frame_queue.get_frame();
	logd("Player.get_img_frame: got a frame from viddec.frame_queue.get_frame()");
	logd("Player.get_img_frame: framesize=%d", viddec.frame_queue.get_size());
	sws_scale(img_convert_ctx, (const uint8_t* const*)av_frame->frame->data, av_frame->frame->linesize, 0, viddec.avctx->height,
		frame->data, frame->linesize);
	double timestamp = video_clock = av_frame_get_best_effort_timestamp(av_frame->frame)*av_q2d(stream_video->time_base);
	this->set_cur_time_video(GetSysTimeMicros());
	double time_wait = this->get_cur_time_video() + av_frame->duration;
	logd("Player.get_img_frame: timestamp=%lf,audio_clock=%lf, cur_time_video=%lf, duration=%lf", timestamp, audio_clock, get_cur_time_video(), av_frame->duration);
	/*if (timestamp > audio_clock) { //在这里把视频同步到音频
		SDL_Delay((unsigned long)((timestamp - audio_clock) * 1000));
		//SDL_Delay((unsigned long)(40));//40ms
	}*/
	while (GetSysTimeMicros() < time_wait) {
		logd("Player.get_img_frame: wait clock");
	}
	av_frame_unref(av_frame->frame);
	av_frame_free(&av_frame->frame);
	return true;
}

bool Player::get_aud_buffer(int &nextSize, uint8_t *outputBuffer) {
	if (outputBuffer == nullptr) return false;
	auto av_frame = auddec.frame_queue.get_frame();
	if (auddec.avctx->sample_fmt == AV_SAMPLE_FMT_S16P) {
		nextSize = av_samples_get_buffer_size(av_frame->frame->linesize, auddec.avctx->channels, auddec.avctx->frame_size, auddec.avctx->sample_fmt, 1);
	}
	else {
		av_samples_get_buffer_size(&nextSize, auddec.avctx->channels, auddec.avctx->frame_size, auddec.avctx->sample_fmt, 1);
	}
	//19200改成 auddec.avctx->frame_size
	int ret = swr_convert(swr_ctx, &outputBuffer, auddec.avctx->frame_size,
		(uint8_t const **)(av_frame->frame->extended_data),
		av_frame->frame->nb_samples);//原先data是extended_data
	double timestamp = audio_clock = av_frame->frame->pkt_pts * av_q2d(stream_audio->time_base); //我想知道只有这里设置了audio_clock吗，应该初始化为0
	this->set_cur_time_audio(GetSysTimeMicros());
	double time_wait = this->get_cur_time_audio() + av_frame->duration;
	logd("Player.get_aud_buffer: timestamp=%lf,audio_clock=%lf, cur_time_audio=%lf, duration=%lf", timestamp, audio_clock, get_cur_time_audio(), av_frame->duration);
	while (GetSysTimeMicros() < time_wait) {
		logd("Player.get_aud_buffer: wait clock");
	}
	av_frame_unref(av_frame->frame);
	av_frame_free(&av_frame->frame);
	return ret >= 0;
}

void Player::wait_state(PlayerState need_state) {
	logd("Player.wait_state: I am in");
	std::unique_lock<std::mutex> lock(mutex);
	state_condition.wait(lock, [this, need_state] { //类似java的匿名方法
		return this->state >= need_state;
	});
}

void Player::wait_paused() { //用while长询不好吗？ missed 'd'
	std::unique_lock<std::mutex> lock(mutex);
	pause_condition.wait(lock, [this] {
		return !this->paused;
	});
}

void Player::release() {
	if (format_ctx) {
		avformat_close_input(&format_ctx);
	}
}

bool Player::is_playing() {
	return state == PlayerState::PLAYING && !paused;
}

void Player::change_state_to_play() {
	logd("Player.change_state_to_play: I am in");
	if (state == PlayerState::READY) {
		state = PlayerState::PLAYING;
		paused = false;
		logd("Player.change_state_to_play: state has been changed to PLAYING");
	}
	play_when_ready = true;
}

void Player::pause() {
	logd("Player.pause(): current state %d", state);
	if (state == PlayerState::PLAYING) {
		std::unique_lock<std::mutex> lock(mutex);
		paused = true;
		pause_condition.notify_all();
		state = PlayerState::READY;
	}
}

void Player::togglePaused() {
	std::unique_lock<std::mutex> lock(mutex);
	paused = !paused;
	pause_condition.notify_all();
}

bool Player::get_paused() {
	return paused;
}

double Player::get_duration() {
	if (format_ctx != nullptr) {
		return format_ctx->duration / 1000000.0;
	}
}

long Player::get_curr_position() {
	return (long)audio_clock * 1000;
}

double Player::get_progress_bar() {
	return video_clock / (format_ctx->duration / 1000000.0);
}

void Player::stream_seek(int64_t pos) {
	if (!seek_req) {
		seek_pos = pos;
		seek_req = true; //如果上一个请求还没完成就再点击了进度条，那么应该放弃前者的响应直接响应后者，这里没有做到							 
	}
}

void Player::set_event_listener(void(*cb)(int, int, int)) {
	event_listener = cb;
}

void Player::read() { //核心函数 <-init()
	logd("Player.read(): I am in");
	int err, i, ret;

	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	if (pkt == NULL) {
		logd("Player.read(): Could not allocate AVPacket.");
		return;
	}

	int64_t stream_start_time;
	int64_t pkt_ts;
	int pkt_in_play_range = 0;
	int stream_index[AVMEDIA_TYPE_NB];
	memset(stream_index, -1, sizeof(stream_index));

	logd("Player.read(): next line is avformat_alloc_context.");
	this->format_ctx = avformat_alloc_context();
	if (!format_ctx) {
		logd("Player.read(): Could not allocate context.");
		return;
	}

	logd("Player.read(): next line is avformat_open_input, file path %s", file_path);
	err = avformat_open_input(&format_ctx, file_path, NULL, NULL);
	if (err < 0) {
		logd("Player.read(): Could not open input file.");
		release();
	}

	err = avformat_find_stream_info(this->format_ctx, NULL);
	if (err < 0) {
		logd("Player.read(): %s could not find codec parameters", file_path);
		release();
	}
	realtime = is_realtime();
	for (i = 0; i < format_ctx->nb_streams; i++) {
		if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			stream_index[AVMEDIA_TYPE_AUDIO] = i;
		}
		if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			stream_index[AVMEDIA_TYPE_VIDEO] = i;
		}
	}
	if (stream_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		logd("Player.read(): next line open video stream at id %d.", stream_index[AVMEDIA_TYPE_VIDEO]);
		open_stream(stream_index[AVMEDIA_TYPE_VIDEO]);
	}
	if (stream_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		logd("Player.read(): next line open audio stream at id %d.", stream_index[AVMEDIA_TYPE_AUDIO]);
		open_stream(stream_index[AVMEDIA_TYPE_AUDIO]);
	}
	if (video_stream_index< 0 && audio_stream_index < 0) {
		logd("Player.read(): Failed to open file '%s' or configure filtergraph", file_path);
		release();
	}
	while (true) {
		if (abort_request)
			break;
		if (paused)
			av_read_pause(format_ctx);//Pause a network-based stream (e.g. RTSP stream).所以，这种非网络流，能pause吗？也许这个项目的进度条拉动卡顿就是因为这个原因
		else
			av_read_play(format_ctx);//Start playing a network-based stream (e.g. RTSP stream) at the current position.
		if (seek_req) {
			//2413 #define AVSEEK_FLAG_BACKWARD 1 ///< seek backward
			//2414 #define AVSEEK_FLAG_BYTE     2 ///< seeking based on position in bytes
			//2415 #define AVSEEK_FLAG_ANY      4 ///< seek to any frame, even non-keyframes
			//2416 #define AVSEEK_FLAG_FRAME    8 ///< seeking based on frame number
			//经过实验，只有1是最合适的，4会出现花屏，2无法seek，8会seek到后面，如果用0，效果和8差不多
			ret = av_seek_frame(format_ctx, -1, seek_pos * AV_TIME_BASE, 1); //seek函数Seek to the keyframe at timestamp. 'timestamp' in 'stream_index'.
			if (ret < 0) {
				logd("Player.read(): %s: error while seeking", file_path);
			}
			else {
				if (audio_stream_index >= 0) {
					auddec.pkt_queue.flush();//先av_seek_frame(),再flush queue,没有问题吗？2018/3/6/11/33
				}
				if (video_stream_index>= 0) {
					viddec.pkt_queue.flush();
				}
			}
			seek_req = false;
			queue_attachments_req = 1;
			eof = 0;//eof置0没有问题吗？置0就是没有到文件末尾
		}
		ret = av_read_frame(format_ctx, pkt);//Return the next frame of a stream. For audio, it contains an integer number of frames if eachframe has a known fixed size(e.g.PCM or ADPCM data).
		//对应Audio，这个pkt里面也许不止一个frame，那么这就是SDL音频播放崩溃的原因吗？
		if (ret < 0) { //不成功就放一个空的packet，然后把eof置1
			logd("Player.read(): av_read_frame return < 0");
			if ((ret == AVERROR_EOF || avio_feof(format_ctx->pb)) && !eof) {
				if (video_stream_index>= 0)
					viddec.pkt_queue.put_nullpacket();
				if (audio_stream_index >= 0)
					auddec.pkt_queue.put_nullpacket();
				eof = 1;//置1就是到了文件末尾
			}
			if (format_ctx->pb && format_ctx->pb->error)
				break;
		}
		else {
			eof = 0;//置0就是没有到文件末尾，取决于ret的值 -- av_read_frame return 0 if OK, < 0 on error or end of file
		}
		/* check if packet is in change_state_to_play range specified by user, then queue, otherwise discard */
		//discard?这里判断这一帧在不在播放区间内，超过这个区间，就丢弃，而且packet的队列，这里入队的是在read函数内的局部申明的变量，malloc在堆上
		//以下算法是copy与ffplay.c官方代码3032行
		stream_start_time = format_ctx->streams[pkt->stream_index]->start_time;
		pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
		pkt_in_play_range = duration == AV_NOPTS_VALUE ||
			(pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
			av_q2d(format_ctx->streams[pkt->stream_index]->time_base) -
			(double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
			<= ((double)duration / 1000000);
		//logd("Player.read(): duration=%lld, duration == AV_NOPTS_VALUE:%d, start_time=%d", duration, duration == AV_NOPTS_VALUE, start_time);
		if (pkt->stream_index == audio_stream_index && pkt_in_play_range) {//if(0)竟然会进入执行？ndk怎么回事哦
			//这里先屏蔽，因为audio的packetqueue和framequeue都会因为audio部分没有启动而队列满造成入队等待而使此read函数线程卡住从而使video解码也一样卡住
			//此处有个大bug，音频卡顿时，auddec.pkt_queue.put_packet(pkt)卡住，从而视频也卡主，无意中同步了音视频
			if (auddec.pkt_queue.put_packet(pkt) < 0) {
				logd("Player.read(): auddec.pkt_queue.put_packet(pkt) < 0");
				break;
			}
			//logd("Player.read(): stream_start_time=%d,pkt_in_play_range=%d,if=%d", stream_start_time, pkt_in_play_range, pkt->stream_index == audio_stream_index && pkt_in_play_range);
			logd("Player.read(): auddec.pkt_queue.put_packet(pkt) succeed, auddec.pkt_queue.get_queue_size()=%d", auddec.pkt_queue.get_queue_size());
		}
		else if (pkt->stream_index == video_stream_index&& pkt_in_play_range
			&& !(stream_video->disposition & AV_DISPOSITION_ATTACHED_PIC)) {//dispositional [计] 意向的 
			if (viddec.pkt_queue.put_packet(pkt) < 0) {
				logd("Player.read(): viddec.pkt_queue.put_packet(pkt) < 0");
				break;
			}
			//logd("Player.read(): stream_start_time=%d,pkt_in_play_range=%d,if=%d", stream_start_time, pkt_in_play_range, (pkt->stream_index == video_stream_index&& pkt_in_play_range && !(stream_video->disposition & AV_DISPOSITION_ATTACHED_PIC)));
			logd("Player.read(): viddec.pkt_queue.put_packet(pkt) succeed,viddec.pkt_queue.get_queue_size()=%d", viddec.pkt_queue.get_queue_size());
		}
		else {
			av_packet_unref(pkt);
		}
	}
}

bool Player::is_realtime() {//<-init()
	if (format_ctx == nullptr)
		return false;
	if (!strcmp(format_ctx->iformat->name, "rtp")
		|| !strcmp(format_ctx->iformat->name, "rtsp")
		|| !strcmp(format_ctx->iformat->name, "sdp"))
		return true;

	if (format_ctx->pb && (!strncmp(format_ctx->filename, "rtp:", 4)
		|| !strncmp(format_ctx->filename, "udp:", 4)))
		return 1;
	return 0;
}

int Player::open_stream(int stream_index) {//<-read()<-init()
	logd("Player.open_stream: I am in open_stream()");
	AVCodecContext *avctx;
	AVCodec *codec;
	int sample_rate, nb_channels;
	int64_t channel_layout;
	int ret = 0;
	if (stream_index < 0 || stream_index >= format_ctx->nb_streams)
		return -1;
	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return AVERROR(ENOMEM);
	//把format_ctx里的codecpar加载到函数内变量avctx中，这样的话，视频和音频没有共用一个formatcontext，时间戳不会混乱
	//不然播放速度会乘以2
	ret = avcodec_parameters_to_context(avctx, format_ctx->streams[stream_index]->codecpar);
	if (ret < 0) {
		avcodec_free_context(&avctx);
		return AVERROR(ENOMEM);
	}
	//stream的时间戳单位是秒，把packet的时间戳也设置为秒
	av_codec_set_pkt_timebase(avctx, format_ctx->streams[stream_index]->time_base);
	codec = avcodec_find_decoder(avctx->codec_id);
	//存放codec_find_decoder的执行结果
	avctx->codec_id = codec->id;
	eof = 0;
	format_ctx->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	ret = avcodec_open2(avctx, codec, NULL);
	if (ret < 0) {
		logd("Player.open_stream: Fail to open codec on stream %d", stream_index);
		avcodec_free_context(&avctx);
		return ret;
	}
	switch (avctx->codec_type) { //上面是只针对某单个流来配置的avctx，所以这里只有一个case分支会被执行
	case AVMEDIA_TYPE_AUDIO:
		swr_ctx = swr_alloc();
		//44100改为avctx->sample_rate，杂音就消失了，原因是44100 < avctx->sample_rate，会失真
		//但是这里还有另一个问题，为什么用多次avcodec_receive_frame会导致没有声音？如果一个packet的确有多个frame，该怎么处理？ 2018/3/9
		swr_ctx = swr_alloc_set_opts(swr_ctx, AV_CH_LAYOUT_STEREO,
			AV_SAMPLE_FMT_S16, avctx->sample_rate,
			av_get_default_channel_layout(avctx->channels), avctx->sample_fmt, avctx->sample_rate,
			0, NULL);
		if (!swr_ctx || swr_init(swr_ctx) < 0) {
			logd("Player.open_stream: Cannot create sample rate converter for conversion channels!");
			swr_free(&swr_ctx);
			return -1;
		}
		this->audio_stream_index = stream_index;
		this->stream_audio = format_ctx->streams[this->audio_stream_index];
		logd("Player.open_stream --> Player.auddec.init(avctx)");
		auddec.init(avctx);
		logd("Player.open_stream --> Player.auddec.start_decode_thread()");
		auddec.start_decode_thread();
		break;
	case AVMEDIA_TYPE_VIDEO:
		video_stream_index= stream_index;
		stream_video = format_ctx->streams[stream_index];
		img_convert_ctx = sws_getContext(avctx->width, avctx->height, avctx->pix_fmt,
			avctx->width, avctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		logd("Player.open_stream --> viddec.init(avctx)");
		viddec.init(avctx);
		logd("Player.open_stream --> viddec.start_decode_thread()");
		viddec.start_decode_thread();
		break;
	default:
		break;
	}
	logd("Player.open_stream --> Player.change_state(PlayerState::READY)");
	change_state(PlayerState::READY);//只是video那部分ready了，但是这样的话，audio那部分也被误以为ready
	return ret;
}

void Player::change_state(PlayerState state) {
	std::unique_lock<std::mutex> lock(mutex);
	if (state == PlayerState::READY) {
		if (this->auddec.avctx != NULL && this->viddec.avctx != NULL) {
			this->state = state;
		}
		else {
			//如果视频解码器或者音频解码器还没有初始化就不转化状态到PlayerState::READY
		}
	}
	else {
		this->state = state;
	}

	state_condition.notify_all();
}

#endif // !__Player_C__
