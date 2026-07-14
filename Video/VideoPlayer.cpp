#include "VideoPlayer.h"

#include "AudioBackend.h"

#include <chrono>
#include <string.h>

namespace {

double wallClock()
{
	using namespace std::chrono;
	return duration_cast<duration<double> >(steady_clock::now().time_since_epoch()).count();
}

}

/// The soundtrack, once it is playing: a miniaudio sound reading the PCM VideoFile decoded,
/// in place. ma_audio_buffer does not copy the samples, so the vector inside VideoFile stays
/// the one and only copy — and must outlive this, which it does (VideoFile is a member and
/// close() tears them down in order).
struct VideoSound
{
	ma_audio_buffer buffer;
	ma_sound sound;
};

VideoPlayer::VideoPlayer()
: sound_(0)
, playing_(false), paused_(false), endOfStream_(false), frameShown_(false)
, volume_(1.f)
, startTime_(0.0), startPosition_(0.0)
{
}

VideoPlayer::~VideoPlayer()
{
	close();
}

bool VideoPlayer::open(const char* fileName)
{
	close();

	if(!file_.open(fileName))
		return false;

	// The soundtrack goes on the voice bus. That is where the game already put it: the volume
	// the briefing panels drive this with is OPTION_VOICE_VOLUME, gated on OPTION_VOICE_ENABLE
	// (UI_StreamVideo::updateVolume). The full-screen reels stop the music before they start,
	// so their track has the bus to itself either way.
	if(file_.hasAudio() && audio::initialized()){
		VideoSound* sound = new VideoSound;

		ma_audio_buffer_config config = ma_audio_buffer_config_init(
			ma_format_s16, file_.audioChannels(), file_.audioFrames(), file_.audioSamples(), 0);
		config.sampleRate = file_.audioSampleRate();

		if(ma_audio_buffer_init(&config, &sound->buffer) == MA_SUCCESS){
			if(ma_sound_init_from_data_source(audio::engine(), &sound->buffer,
			                                  MA_SOUND_FLAG_NO_SPATIALIZATION,
			                                  audio::group(audio::GROUP_VOICE),
			                                  &sound->sound) == MA_SUCCESS){
				ma_sound_set_volume(&sound->sound, volume_);
				sound_ = sound;
			}
			else{
				ma_audio_buffer_uninit(&sound->buffer);
				delete sound;
			}
		}
		else
			delete sound;
	}

	return true;
}

void VideoPlayer::close()
{
	if(sound_){
		ma_sound_uninit(&sound_->sound);
		ma_audio_buffer_uninit(&sound_->buffer);
		delete sound_;
		sound_ = 0;
	}

	file_.close();

	playing_ = paused_ = endOfStream_ = frameShown_ = false;
	startTime_ = startPosition_ = 0.0;
}

void VideoPlayer::play()
{
	if(!opened())
		return;

	paused_ = false;
	playing_ = true;

	startTime_ = wallClock();

	if(sound_)
		ma_sound_start(&sound_->sound);
}

void VideoPlayer::stop()
{
	if(!opened())
		return;

	playing_ = false;
	paused_ = false;

	if(sound_)
		ma_sound_stop(&sound_->sound);

	rewind();
}

void VideoPlayer::pause(bool pause)
{
	if(!opened() || paused_ == pause)
		return;

	paused_ = pause;

	if(sound_){
		// The cursor stays where it is, so the clock pauses with the sound.
		if(pause)
			ma_sound_stop(&sound_->sound);
		else if(playing_)
			ma_sound_start(&sound_->sound);
	}
	else{
		// The wall clock does not stop by itself: bank the position we paused at, and start
		// counting again from there.
		if(pause)
			startPosition_ = clock();
		else
			startTime_ = wallClock();
	}
}

void VideoPlayer::setVolume(float volume)
{
	volume_ = volume < 0.f ? 0.f : (volume > 1.f ? 1.f : volume);

	if(sound_)
		ma_sound_set_volume(&sound_->sound, volume_);
}

double VideoPlayer::clock() const
{
	if(sound_){
		float cursor = 0.f;
		if(ma_sound_get_cursor_in_seconds(const_cast<ma_sound*>(&sound_->sound), &cursor) == MA_SUCCESS)
			return cursor;
		return 0.0;
	}

	if(!playing_ || paused_)
		return startPosition_;

	return startPosition_ + (wallClock() - startTime_);
}

void VideoPlayer::rewind()
{
	file_.rewind();

	endOfStream_ = false;
	frameShown_ = false;
	startPosition_ = 0.0;
	startTime_ = wallClock();

	if(sound_){
		ma_sound_seek_to_pcm_frame(&sound_->sound, 0);
		// A sound that reached its end stopped itself, and seeking does not undo that — so a
		// looping video would rewind into silence without this.
		if(playing_ && !paused_)
			ma_sound_start(&sound_->sound);
	}
}

float VideoPlayer::phase() const
{
	const float duration = file_.duration();
	if(!opened() || duration <= 0.f)
		return 0.f;

	const float phase = (float)(clock() / duration);
	return phase < 0.f ? 0.f : (phase > 1.f ? 1.f : phase);
}

void VideoPlayer::setPhase(float phase)
{
	if(!opened())
		return;

	if(phase <= 0.f){
		rewind();
		return;
	}

	const double position = phase * file_.duration();

	file_.seek(position);
	endOfStream_ = false;
	frameShown_ = false;

	if(sound_){
		ma_sound_seek_to_pcm_frame(&sound_->sound,
		                           (ma_uint64)(position * file_.audioSampleRate()));
		if(playing_ && !paused_)
			ma_sound_start(&sound_->sound);
	}
	else{
		startPosition_ = position;
		startTime_ = wallClock();
	}
}

void VideoPlayer::copyFrameTo(unsigned char* destination, int pitch) const
{
	const unsigned char* frame = frameBGRA();
	if(!frame || !destination)
		return;

	const int rowBytes = width() * 4;
	const int rows = height();

	// Same byte order on both sides (BGRA), so this is a copy and not a conversion — but the
	// destination's pitch is the device's business, not ours, so go row by row.
	for(int y = 0; y < rows; ++y)
		memcpy(destination + (size_t)y * pitch, frame + (size_t)y * rowBytes, rowBytes);
}

bool VideoPlayer::quant()
{
	if(!opened() || !playing_ || paused_ || endOfStream_)
		return false;

	const double now = clock();
	const double frameTime = file_.frameTime();

	bool decoded = false;

	// The first frame is due immediately; after that, one is due once the clock has passed the
	// end of the one on screen. Every frame gets decoded — a Bink frame is a delta against its
	// predecessor, so there is no skipping ahead, only decoding faster. A hitch in the game
	// therefore comes back as a short burst of decodes here rather than as drift.
	while(!frameShown_ || file_.framePosition() + frameTime <= now){
		if(!file_.decodeFrame()){
			endOfStream_ = true;
			break;
		}

		frameShown_ = true;
		decoded = true;

		if(frameTime <= 0.0)   // a file that reports no frame rate: one frame per quant
			break;
	}

	return decoded;
}
