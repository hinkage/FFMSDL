#ifndef __Player_H__
#define __Player_H__
#include "AudioDecoder.h"
#include "VideoDecoder.h"

class Player {
public:
	Player();
	void set_cur_time_video(double t);
	double get_cur_time_video();	
	void set_cur_time_audio(double t);
	double get_cur_time_audio();
	void set_file_path(const std::string input_filename);
	void init(const std::string input_filename);
	void start_read_thread();
	bool has_video();
	bool get_img_frame(AVFrame *frame);
	bool get_aud_buffer(int &nextSize, uint8_t *outputBuffer);
	void wait_state(PlayerState need_state);
	void wait_paused();
	void release();
	bool is_playing();
	void change_state_to_play();
	void pause();
	void togglePaused();
	bool get_paused();
	double get_duration();
	long get_curr_position();
	void stream_seek(int64_t pos);
	void set_event_listener(void(*cb)(int, int, int));

	double get_progress_bar();

	AVFormatContext *format_ctx;
	char *file_path;
	int abort_request=0;
	int force_refresh;

	int last_paused;
	int queue_attachments_req;
	bool seek_req = false;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel; //?
	int read_pause_return;
	bool realtime;
	AudioDecoder auddec;
	VideoDecoder viddec;
	PlayerState state = PlayerState::UNKNOWN;

	struct SwsContext *img_convert_ctx;
private:
	void read();
	bool is_realtime();
	int open_stream(int stream_index);
	void change_state(PlayerState state);

	double cur_time_video = 0;
	double cur_time_audio = 0;

	int last_video_stream, last_audio_stream;
	int video_stream_index= -1;
	AVStream *stream_video;
	bool paused = false;
	bool play_when_ready = false;

	void(*event_listener)(int, int, int);

	double max_frame_duration; // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
							   //struct SwsContext *img_convert_ctx;//移动到public区

	double video_clock = 0;
	double audio_clock = 0;
	int audio_clock_serial;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *stream_audio;

	int audio_hw_buf_size;
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	unsigned int audio_buf1_size;
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	int audio_volume;
	int muted;
	struct SwrContext *swr_ctx;
	int frame_drops_early;
	int frame_drops_late;
	int eof;

	int audio_stream_index = -1;
	int av_sync_type;

	int64_t start_time = AV_NOPTS_VALUE;
	int64_t duration = AV_NOPTS_VALUE;
	std::mutex mutex;
	std::condition_variable state_condition;
	std::condition_variable pause_condition;

	static const int MEDIA_NOP = 0; // interface test message
	static const int MEDIA_PREPARED = 1;
	static const int MEDIA_PLAYBACK_COMPLETE = 2;
	static const int MEDIA_BUFFERING_UPDATE = 3;
	static const int MEDIA_SEEK_COMPLETE = 4;
	static const int MEDIA_SET_VIDEO_SIZE = 5;
	static const int MEDIA_TIMED_TEXT = 99;
	static const int MEDIA_ERROR = 100;
	static const int MEDIA_INFO = 200;
};
#endif // !__Player_H__

