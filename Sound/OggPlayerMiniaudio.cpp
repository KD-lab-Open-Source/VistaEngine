// OggPlayer on miniaudio: the music, the speech, and the cutscene audio.
//
// The original (XLibs.Net/OGG/PlayOgg) is still in the tree as the reference. It ran a streaming
// thread of its own, refilling a DirectSound buffer from libvorbis and ticking its own fade — all
// of which miniaudio does for us. A track becomes one streaming ma_sound: decoded a page at a time
// off the engine's VFS (so speech plays straight out of voices_*.pak) by the Vorbis backend in
// Sound/VorbisDecoder.cpp, faded on the audio thread, and looped by seeking.
//
// The parts of the interface the game never calls — OggStream, length(), tell(), fileName(),
// setAutoFade(), setPauseIfZeroVolume() — are not implemented, exactly as they were not before.
#include "StdAfx.h"
#include "AudioBackend.h"
#include "PlayOgg.h"
#include "Console.h"

#include <string>

namespace {

// Music and speech ran through the very same curve the effects did: PlayOgg's ToDirectVolume() and
// the game's CalcVolume() are the same function written twice (both are
// DB_MIN + 10000*log10(ln(vol+1)*1.623031921 + 1), and 9/(ln2*8) is that constant). So the volume a
// track is given, 0..255, becomes gain the same way — see soundGain() in SoundSystemMiniaudio.cpp,
// which this mirrors.
float musicGain(int volume)
{
	if(volume <= 0)
		return 0.0f;

	int vol = (volume > 255) ? 255 : volume;
	double t = log(double(vol + 1))*1.623031921 + 1.0;

	return (float)(t*t*t*t*t*1e-5);
}

}

/// One streaming track.
class OggPlayerImpl
{
public:
	OggPlayerImpl()
	{
		bus_ = OGG_BUS_MUSIC;
		state_ = OGG_STOPPED;
		volume_ = 255;
		playing_ = false;
		fadeEndTime_ = 0;
	}

	~OggPlayerImpl()
	{
		stop();
	}

	void setBus(OggBus bus)
	{
		bus_ = bus;
	}

	bool play(const char* fname, bool cycled)
	{
		stop();

		if(!fname || !audio::initialized())
			return false;

		// MA_SOUND_FLAG_STREAM: decode as it plays. The music is 58 MB of Vorbis across the
		// soundtrack and a track would be some 80 MB of PCM if it were decoded up front.
		//
		// The path is the one out of the script tables ("Resource\Music\Battle.ogg", or a line of
		// speech inside voices_en.pak) and the VFS resolves either.
		ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION;
		ma_sound_group* group = audio::group(bus_ == OGG_BUS_VOICE ? audio::GROUP_VOICE : audio::GROUP_MUSIC);

		if(ma_sound_init_from_file(audio::engine(), fname, flags, group, 0, &sound_) != MA_SUCCESS){
			kdWarning("&OggPlayer", XBuffer(1024, 1) < /*TRANSLATE*/("Невозможно открыть файл : ") < fname);
			return false;
		}

		playing_ = true;
		fileName_ = fname;
		fadeEndTime_ = 0;

		ma_sound_set_looping(&sound_, cycled);
		ma_sound_set_volume(&sound_, musicGain(volume_));

		if(ma_sound_start(&sound_) != MA_SUCCESS){
			ma_sound_uninit(&sound_);
			playing_ = false;
			return false;
		}

		state_ = OGG_PLAYING;
		return true;
	}

	void stop()
	{
		if(!playing_)
			return;

		if(audio::initialized())
			ma_sound_uninit(&sound_);

		playing_ = false;
		state_ = OGG_STOPPED;
		fadeEndTime_ = 0;
		fileName_.clear();
	}

	void pause()
	{
		if(state_ != OGG_PLAYING)
			return;

		ma_sound_stop(&sound_);   // stop, not uninit: the cursor stays where it is
		state_ = OGG_PAUSED;
	}

	void resume()
	{
		if(state_ != OGG_PAUSED)
			return;

		ma_sound_start(&sound_);
		state_ = OGG_PLAYING;
	}

	OggState state()
	{
		// A track that has played itself out is stopped, whatever we last thought. The briefings
		// hang on this: a message waits for the speech to finish by watching isPlaying().
		if(state_ == OGG_PLAYING && playing_ && ma_sound_at_end(&sound_))
			state_ = OGG_STOPPED;

		return state_;
	}

	void setVolume(int volume)
	{
		volume_ = volume;

		// Mid-fade the fade owns the volume, as it did originally: setVolume() only recorded the
		// level it would land on.
		if(playing_ && !fading())
			ma_sound_set_volume(&sound_, musicGain(volume_));
	}

	int volume() const
	{
		return volume_;
	}

	/// new_volume is relative to the player's own volume: 0 fades out, 1 fades back in.
	bool fadeVolume(float time, float new_volume)
	{
		if(!playing_)
			return false;

		if(time <= 0.05f){
			fadeEndTime_ = 0;
			ma_sound_set_volume(&sound_, musicGain(round(volume_*new_volume)));
			return false;
		}

		// miniaudio fades on the audio thread, so there is no quant of ours to run — the original
		// ticked this from its streaming thread. It interpolates the gain where the original
		// interpolated the 0..255 volume and mapped that through the curve, but the two run within
		// a few percent of each other across the whole range (the curve is very nearly linear in
		// gain), and both begin and end on exactly the same value.
		int ms = round(time*1000.0f);
		ma_sound_set_fade_in_milliseconds(&sound_, ma_sound_get_volume(&sound_),
		                                  musicGain(round(volume_*new_volume)), ms);
		fadeEndTime_ = xclock() + ms;

		return true;
	}

private:
	bool fading() const
	{
		return fadeEndTime_ != 0 && xclock() < fadeEndTime_;
	}

	ma_sound sound_;
	OggBus bus_;
	OggState state_;
	std::string fileName_;
	int volume_;      ///< 0..255, as the game gives it
	bool playing_;    ///< is sound_ initialized
	int fadeEndTime_;
};

//-----------------------------------------------------------------------------
// OggPlayer (PlayOgg.h)
//-----------------------------------------------------------------------------
OggPlayer::OggPlayer()
: player_(new OggPlayerImpl)
{
}

OggPlayer::~OggPlayer()
{
	delete player_;
}

bool OggPlayer::play(const char* fname, bool cycled, void* /*data_handle*/)
{
	// data_handle was the XZipStream the original streamed through, by way of the callbacks
	// installed with setCallbacks(). The VFS does that job for every sound now, so the handle is
	// not needed and the callbacks are not either.
	return player_->play(fname, cycled);
}

void OggPlayer::stop()
{
	player_->stop();
}

void OggPlayer::pause()
{
	player_->pause();
}

void OggPlayer::resume()
{
	player_->resume();
}

void OggPlayer::setBus(OggBus bus)
{
	player_->setBus(bus);
}

OggState OggPlayer::state() const
{
	return player_->state();
}

void OggPlayer::setVolume(int volume)
{
	player_->setVolume(volume);
}

int OggPlayer::volume() const
{
	return player_->volume();
}

bool OggPlayer::fadeVolume(float time, float new_volume)
{
	return player_->fadeVolume(time, new_volume);
}

void OggPlayer::setCallbacks(OggCallbacks*)
{
	// Nothing to install: the VFS reads every sound in the game, archives included.
}

/// How long a line of speech runs, in seconds — the briefings pace themselves by it.
double OggPlayer::getLength(const char* fname)
{
	if(!fname || !audio::initialized())
		return 0.0;

	ma_sound sound;
	if(ma_sound_init_from_file(audio::engine(), fname,
	                           MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION,
	                           0, 0, &sound) != MA_SUCCESS)
		return 0.0;

	float length = 0.0f;
	ma_sound_get_length_in_seconds(&sound, &length);
	ma_sound_uninit(&sound);

	return length;
}

bool OggPlayer::initLibrary(void* engine)
{
	// The device is already up by the time this is called (Game/SoundApp.cpp hands us its engine);
	// there is no separate library to start, and no streaming thread to spawn.
	return engine != 0;
}

void OggPlayer::finitLibrary()
{
}
