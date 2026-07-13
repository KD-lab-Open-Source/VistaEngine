// SoundSystem, Sound and Channel on miniaudio.
//
// The DirectSound original is still in the tree, unbuilt, as the reference: SoundSystem.cpp
// (this file), WaveFile.cpp, C3D.cpp, SoftwareBuffer.cpp, HardwareBuffer.cpp, and init.cpp
// (the SND* functions at the bottom). Read it before porting a behaviour rather than
// inventing one — the shape here follows it closely, deliberately.
//
// What is real: the sample pool and its voices. A Sound is one decoded sample; a Channel is
// one voice playing it, with its own cursor, volume and fade. Looping, pausing, muting and
// the global fade all behave as they did.
//
// What is not, yet: spatialization. Every voice is played unspatialized, so a 3D sound is
// audible but not placed, and the listener, the distance attenuation, the clip-distance pass
// and the fog-of-war muting are all still no-ops. That is the next slice; the sites are
// tagged TODO(sound-port). OggPlayer and SND3DListener are still in SoundStub.cpp.
#include "StdAfx.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "AudioBackend.h"
#include "SystemUtil.h"

SoundSystem sndSystem;

namespace {

// DirectSound took volume as millibels of attenuation, and the game fed it through
// CalcVolume(): a 0..255 value on a logarithmic curve. miniaudio wants linear gain, so fold
// the two together. CalcVolume gives
//
//     mB = DSBVOLUME_MIN + 10000*log10(t),  t = ln(vol + 1)*1.623031921 + 1
//
// and DirectSound's gain is 10^(mB/2000) with DSBVOLUME_MIN = -10000, which collapses to
//
//     gain = t^5 / 100000
//
// (t is 1 at vol=0 and 10 at vol=255, so this runs from 1e-5 to 1.) Reproducing the curve
// rather than fading linearly matters: it is what every volume in the sound library was
// mixed against, and it is audibly louder than a linear fader across most of its range.
float soundGain(float volume)
{
	if(volume <= 0.0f)
		return 0.0f;

	int vol = (int)(clamp(volume, 0.0f, 1.0f)*255.0f);
	double t = log(double(vol + 1))*1.623031921 + 1.0;

	return (float)(t*t*t*t*t*1e-5);
}

}

//-----------------------------------------------------------------------------
// SoundSystem — the device, and the sounds loaded on it
//-----------------------------------------------------------------------------
SoundSystem::SoundSystem()
{
	globalVolume_ = 1.0f;
	globalFadeFactor_ = 1.0f;
	enable_ = false;
	mute3Dsounds_ = false;
	gameActive_ = false;
	numberOfUsedSounds_ = 0;
	numberOfPlayingSounds_ = 0;
	fadeTime = 0.0f;
	needVolumeUpdate = false;
	globalFadeIn = false;
	prevTime_ = 0;
	standbyTime_ = 0.0f;
	needFade_ = false;
	fadeAllSounds_ = true;
}

SoundSystem::~SoundSystem()
{
	Release();
}

bool SoundSystem::Init()
{
	if(!audio::init())
		return false;

	prevTime_ = xclock();
	return true;
}

void SoundSystem::Release()
{
	// The sounds go before the engine that owns their voices. Note that sndLibrary indexes
	// these very Sounds by name, so it has to let go of them first — FinitSound() does that,
	// on the Game side, because the name index is not this module's to know about.
	for(int i = 0; i < sounds_.size(); i++){
		delete sounds_[i];
		sounds_[i] = 0;
	}
	sounds_.clear();

	audio::release();
}

void* SoundSystem::GetAudioEngine()
{
	return audio::engine();
}

Sound* SoundSystem::CreateSound(const char* filename, DWORD mode)
{
	Sound* sound = new Sound(this);
	if(!sound->CreateSoundFromFile(filename, mode)){
		delete sound;
		return 0;
	}

	sounds_.push_back(sound);
	return sound;
}

void SoundSystem::Update()
{
	// DirectSound at a normal cooperative level silenced itself when the window lost focus.
	// miniaudio keeps playing to the device regardless, so the mute has to be ours.
	bool focus = applicationHasFocus();
	if(focus == audio::muted())
		audio::setMuted(!focus);

	if(!focus)
		return;

	start_timer_auto();

	needVolumeUpdate = false;

	int curTime = xclock();
	float dt = float(curTime - prevTime_)*0.001f;
	if(dt > 0.1f)
		dt = 0.1f;
	prevTime_ = curTime;

	if(standbyTime_ > 0){
		standbyTime_ -= dt;
		return;
	}

	if(needFade_){
		globalFadeFactor_ += dt*fadeTime*(globalFadeIn ? 1 : -1);
		if(globalFadeFactor_ > 1){
			fadeTime = 0;
			globalFadeFactor_ = 1;
			needFade_ = false;
		}
		if(globalFadeFactor_ < 0){
			fadeTime = 0;
			globalFadeFactor_ = 0;
			needFade_ = false;
		}
		needVolumeUpdate = true;
	}

	int total_use = 0;
	int total_play = 0;

	for(int i = 0; i < sounds_.size(); i++){
		Sound* sound = sounds_[i];
		if(!sound || sound->isFree())
			continue;

		sound->Update(dt);
		total_use += sound->getUsingSoundsCount();
		total_play += sound->getPlayingSoundsCount();
	}

	numberOfPlayingSounds_ = total_play;
	numberOfUsedSounds_ = total_use;
}

void SoundSystem::EnableSound(bool enable)
{
	if(enable == enable_)
		return;

	enable_ = enable;
	MuteAll(!enable_);
}

bool SoundSystem::IsEnabled()
{
	return enable_;
}

// "Game active" means a mission is running, not that the window has focus: it gates the
// fog-of-war muting, and nothing else.
void SoundSystem::SetGameActive(bool active)
{
	gameActive_ = active;
}

void SoundSystem::SetGlobalVolume(float volume)
{
	globalVolume_ = volume;

	// Every voice recomputes its own gain: the whole product (global * channel * fade) goes
	// through soundGain() as one value, exactly as CalcVolume() was fed. That is also why
	// the SFX group stays at unity — the curve is not multiplicative, so splitting the
	// global term onto the group bus would not give the same level.
	for(int i = 0; i < sounds_.size(); i++)
		if(sounds_[i])
			sounds_[i]->UpdateVolume();
}

float SoundSystem::GetGlobalVolume()
{
	return globalVolume_;
}

int SoundSystem::numberOfPlayingSounds()
{
	return numberOfPlayingSounds_;
}

int SoundSystem::numberOfUsedSounds()
{
	return numberOfUsedSounds_;
}

void SoundSystem::StopAll()
{
	for(int i = 0; i < sounds_.size(); i++)
		if(sounds_[i])
			sounds_[i]->StopAll();
}

void SoundSystem::MuteAll(bool mute)
{
	for(int i = 0; i < sounds_.size(); i++){
		if(!sounds_[i])
			continue;

		if(mute)
			sounds_[i]->MuteAll(true);
		else if(!mute3Dsounds_ || !sounds_[i]->Is3DSound())
			sounds_[i]->MuteAll(false);
	}
}

void SoundSystem::Mute3DSounds(bool mute)
{
	StartFade(!mute, 1000, false);
}

void SoundSystem::RecalculateClipDistance()
{
	for(int i = 0; i < sounds_.size(); i++)
		if(sounds_[i])
			sounds_[i]->RecalculateClipDistance();
}

void SoundSystem::StartFade(bool fadeIn, int time, bool allSounds)
{
	if(fadeIn && !allSounds && fadeAllSounds_)
		fadeAllSounds_ = true;
	else
		fadeAllSounds_ = allSounds;

	if(time == 0){
		globalFadeFactor_ = fadeIn ? 1.0f : 0.0f;
		SetGlobalVolume(globalVolume_);
		return;
	}

	fadeTime = 1000.0f/(float)time;
	globalFadeIn = fadeIn;
	needFade_ = true;
}

void SoundSystem::SetStandbyTime(float time)
{
	standbyTime_ = time;
}

//-----------------------------------------------------------------------------
// Sound — one decoded sample, and the pool of voices playing it
//-----------------------------------------------------------------------------
Sound::Sound(SoundSystem* system)
{
	system_ = system;
	source_ = 0;
	mode_ = 0;
	// The original started this at DSBVOLUME_MAX, meaning "loudest" — but DSBVOLUME_MAX is 0,
	// and 0 is what it then fed to a 0..1 volume scale, so an unconfigured Sound was actually
	// silent. Every Sound is configured by its SoundAttribute the moment it is created, so it
	// never showed; start at the volume it meant.
	volume_ = 1.0f;
	minDistance_ = 100.0f;
	maxDistance_ = 500000.0f;
	maxChannels_ = 25;
	numUsedChanel = 0;
	numPlayedChanel = 0;
	isFree_ = true;
	stopInFogOfWar_ = true;
	useGlobalFade_ = true;
}

Sound::~Sound()
{
	Release();
}

bool Sound::CreateSoundFromFile(const char* filename, DWORD mode)
{
	mode_ = mode;

	if(!audio::initialized())
		return false;

	source_ = new ma_sound;

	// MA_SOUND_FLAG_DECODE: decode the whole sample up front, once. Every channel is then a
	// copy sharing that one decoded buffer (ma_sound_init_copy), which is what
	// DuplicateSoundBuffer bought us before. These are short mono effects; nothing to stream.
	//
	// The path is the archive-relative one straight out of the sound library
	// ("Resource\Sounds\x.wav") and the engine's VFS resolves it (Sound/AudioBackend.cpp) —
	// which it must, because the effects exist nowhere but inside sound.pak.
	ma_result result = ma_sound_init_from_file(audio::engine(), filename,
	                                           MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION,
	                                           0, 0, source_);
	if(result != MA_SUCCESS){
		fprintf(stderr, "Sound: cannot open '%s' (miniaudio result %i)\n", filename, (int)result);
		delete source_;
		source_ = 0;
		return false;
	}

	return true;
}

void Sound::Release()
{
	MTAuto lock(criticalSection);

	for(int i = 0; i < channels_.size(); i++){
		delete channels_[i];
		channels_[i] = 0;
	}
	channels_.clear();

	if(source_){
		// A Sound is normally deleted by SoundSystem::Release, while the engine is still up.
		// Guard anyway: one held somewhere that outlives the device must not hand its sample
		// back to an engine that is already gone.
		if(audio::initialized())
			ma_sound_uninit(source_);

		delete source_;
		source_ = 0;
	}
}

void Sound::SetVolume(float volume)
{
	volume_ = volume;
}

bool Sound::Is3DSound()
{
	return !(mode_ & SS_2DSOUND);
}

void Sound::SetMaxChannels(int maxChannels)
{
	maxChannels_ = maxChannels;
}

void Sound::Set3DMinMaxDistance(float min, float max)
{
	minDistance_ = min;
	maxDistance_ = max;
}

void Sound::Get3DMinMaxDistance(float* min, float* max)
{
	if(min)
		*min = minDistance_;
	if(max)
		*max = maxDistance_;
}

void Sound::StopAll()
{
	MTAuto lock(criticalSection);

	for(int i = 0; i < channels_.size(); i++)
		if(channels_[i])
			channels_[i]->Stop(true);
}

void Sound::MuteAll(bool mute)
{
	MTAuto lock(criticalSection);

	for(int i = 0; i < channels_.size(); i++)
		if(channels_[i])
			channels_[i]->SetMute(mute);
}

void Sound::UpdateVolume()
{
	MTAuto lock(criticalSection);

	for(int i = 0; i < channels_.size(); i++)
		if(channels_[i])
			channels_[i]->UpdateVolume();
}

void Sound::Update(float dt)
{
	std::vector<Channel*> local_channels;
	{
		MTAuto lock(criticalSection);
		local_channels = channels_;
	}

	numUsedChanel = numPlayedChanel = 0;
	int freeCount = 0;

	for(int i = 0; i < local_channels.size(); i++){
		Channel* channel = local_channels[i];
		if(!channel)
			continue;

		if(channel->isFree()){
			freeCount++;
			continue;
		}

		channel->Update(dt);

		if(channel->IsPlaying())
			numPlayedChanel++;
		if(channel->IsUsed())
			numUsedChanel++;
	}

	if(freeCount == local_channels.size())
		isFree_ = true;
}

void Sound::RecalculateClipDistance()
{
	if(mode_ & SS_2DSOUND)
		return;
	if(!system_->enable_)
		return;

	// TODO(sound-port): with no listener there is no distance to sort by, so every channel
	// stays audible and maxChannels_ caps nothing. The original ranked the playing channels by
	// distance, muted the ones past maxDistance_, and paused everything past maxChannels_.
	// Restore it with the 3D slice.
}

Channel* Sound::CreateAndPlayChannel(bool paused)
{
	Channel* chnl = FindFreeChannel();
	if(!chnl)
		return 0;

	chnl->Set3DMinMaxDistance(minDistance_, maxDistance_);
	chnl->SetUseGlobalFade(useGlobalFade_);
	chnl->SetStopInFogOfWar(stopInFogOfWar_);
	chnl->SetVolume(volume_);
	chnl->isUsed_ = true;

	if(paused){
		chnl->SetPaused(true);
		return chnl;
	}

	chnl->Play();
	return chnl;
}

bool Sound::PlaySound(const Vect3f& pos)
{
	Channel* chnl = FindFreeChannel();
	if(!chnl)
		return false;

	chnl->Set3DMinMaxDistance(minDistance_, maxDistance_);
	chnl->SetUseGlobalFade(useGlobalFade_);
	chnl->SetStopInFogOfWar(stopInFogOfWar_);
	chnl->SetVolume(volume_);
	// isUsed_ stays false: this is a fire-and-forget one-shot, so the channel returns itself
	// to the pool the moment it finishes.
	chnl->isUsed_ = false;
	chnl->SetLoop(false);
	chnl->SetPosition(pos);
	chnl->SetPan(0.5f);
	chnl->Play();

	return true;
}

Channel* Sound::FindFreeChannel()
{
	MTAuto lock(criticalSection);

	for(int i = 0; i < channels_.size(); i++){
		if(!channels_[i]->IsUsed() && !channels_[i]->IsPlaying()){
			channels_[i]->isFree_ = false;
			isFree_ = false;
			return channels_[i];
		}
	}

	Channel* channel = new Channel(this);
	if(!channel->Init()){
		delete channel;
		return 0;
	}

	channels_.push_back(channel);
	return channel;
}

//-----------------------------------------------------------------------------
// Channel — one voice
//-----------------------------------------------------------------------------
Channel::Channel(Sound* sound)
{
	voice_ = 0;
	needDelete_ = false;
	sound_ = sound;
	isUsed_ = false;
	paused_ = false;
	isLooped_ = false;
	is3DSound_ = sound->mode_ & SS_3DSOUND;
	position_ = Vect3f::ZERO;
	minDistance_ = 0.0f;
	maxDistance_ = 0.0f;
	maxDistance2_ = 0.0f;
	volume_ = 1.0f;
	playing_ = false;
	fadeOut_ = 0;
	fadeIn_ = 0;
	isMuted_ = false;
	fadeTime_ = 0;
	isFadeIn_ = true;
	needStop_ = false;
	fadeVolumeFactor_ = 1;
	inFogOfWar_ = false;
	stopInFogOfWar_ = true;
	useGlobalFade_ = true;
	needFade_ = false;
	isFree_ = true;
	canPlay_ = true;
}

Channel::~Channel()
{
	Release();

	if(voice_){
		if(audio::initialized())
			ma_sound_uninit(voice_);

		delete voice_;
		voice_ = 0;
	}
}

bool Channel::Init()
{
	if(!sound_->source_ || !audio::initialized())
		return false;

	voice_ = new ma_sound;

	// A copy shares the sample's decoded data but gets its own cursor, volume and pan — the
	// same relationship DuplicateSoundBuffer gave a channel.
	//
	// TODO(sound-port): MA_SOUND_FLAG_NO_SPATIALIZATION unconditionally, so a 3D sound plays
	// but is not placed. The 3D slice makes this conditional on is3DSound_.
	ma_result result = ma_sound_init_copy(audio::engine(), sound_->source_,
	                                      MA_SOUND_FLAG_NO_SPATIALIZATION,
	                                      audio::group(audio::GROUP_SFX), voice_);
	if(result != MA_SUCCESS){
		fprintf(stderr, "Channel: cannot create voice (miniaudio result %i)\n", (int)result);
		delete voice_;
		voice_ = 0;
		return false;
	}

	fadeVolumeFactor_ = 0;
	isFree_ = false;
	sound_->isFree_ = false;

	return true;
}

void Channel::Release()
{
	isUsed_ = false;
}

void Channel::Play()
{
	if(!sound_->system_->enable_ || !voice_)
		return;

	Apply3DParameters();
	ma_sound_seek_to_pcm_frame(voice_, 0);

	paused_ = false;
	isMuted_ = false;
	playing_ = true;

	// TODO(sound-port): the original muted a 3D channel here if it started further away than
	// maxDistance_. No listener yet, so no distance.

	sound_->RecalculateClipDistance();

	if(canPlay_)
		BufferPlay(true);
}

void Channel::BufferPlay(bool fromZero)
{
	playing_ = true;

	if(isMuted_)
		return;

	if(!isInFogOfWar()){
		if(fadeIn_ > 0){
			StartFade(true, fadeIn_);
			if(fromZero)
				fadeVolumeFactor_ = 0;
		}else
			fadeVolumeFactor_ = 1;

		ma_sound_set_looping(voice_, isLooped_);
		ma_sound_start(voice_);
	}else
		inFogOfWar_ = true;

	ChangeVolume(volume_*fadeVolumeFactor_);
}

void Channel::BufferStop(bool immediately)
{
	if(fadeOut_ <= 0 || immediately){
		ma_sound_stop(voice_);
		fadeVolumeFactor_ = 0;
	}else
		StartFade(false, fadeOut_);
}

void Channel::Stop(bool immediately)
{
	if(!playing_)
		return;

	BufferStop(immediately);

	if(fadeOut_ <= 0 || immediately){
		playing_ = false;
		paused_ = false;
		isMuted_ = false;
	}else
		needStop_ = true;
}

// Only a looping sound can be paused: a one-shot is left to finish.
void Channel::SetPaused(bool pause)
{
	if(!isUsed_ || pause == paused_)
		return;
	if(!isLooped_)
		return;

	paused_ = pause;

	if(pause)
		BufferStop();
	else
		BufferPlay();
}

void Channel::SetLoop(bool loop)
{
	isLooped_ = loop;
}

void Channel::SetMute(bool mute)
{
	if(mute == isMuted_)
		return;

	isMuted_ = mute;

	if(mute)
		BufferStop();
	else if(isUsed_ && isLooped_)
		BufferPlay();
}

void Channel::StartFade(bool fadeIn, int time)
{
	fadeTime_ = 1000.0f/time;
	isFadeIn_ = fadeIn;
	needFade_ = true;
}

void Channel::Update(float dt)
{
	start_timer_auto();

	if(!voice_)
		return;

	if(sound_->system_->needVolumeUpdate)
		UpdateVolume();

	CheckFogOfWar();

	if(needFade_){
		fadeVolumeFactor_ += (dt*fadeTime_)*(isFadeIn_ ? 1 : -1);

		if(fadeVolumeFactor_ < 0){
			fadeTime_ = 0;
			needFade_ = false;
			fadeVolumeFactor_ = 0;
			ma_sound_stop(voice_);
			if(needStop_)
				playing_ = false;
			needStop_ = false;
		}
		if(fadeVolumeFactor_ > 1){
			fadeTime_ = 0;
			needFade_ = false;
			fadeVolumeFactor_ = 1;
		}

		ChangeVolume(volume_*fadeVolumeFactor_);
	}

	if(playing_ && !paused_ && !isMuted_ && !inFogOfWar_){
		if(!isBufferPlaying())
			playing_ = false;
	}
	if(playing_ && (paused_ || isMuted_ || inFogOfWar_) && !isLooped_)
		playing_ = false;
	if(playing_ && !isUsed_ && isLooped_ && !needFade_)
		playing_ = false;

	// Nobody holds it and nothing is coming out of it: back to the pool.
	if(!playing_ && !isUsed_)
		isFree_ = true;
}

bool Channel::isBufferPlaying()
{
	// A one-shot that ran to the end is flagged atEnd and stopped on the next processing step,
	// so ask both: the flag lands first.
	return ma_sound_is_playing(voice_) && !ma_sound_at_end(voice_);
}

bool Channel::IsPlaying()
{
	return playing_;
}

bool Channel::IsUsed()
{
	return voice_ && isUsed_;
}

void Channel::ChangeVolume(float volume)
{
	if(!voice_)
		return;

	float fVol = sound_->system_->globalVolume_*volume;
	if(useGlobalFade_ || sound_->system_->fadeAllSounds_)
		fVol *= sound_->system_->globalFadeFactor_;

	ma_sound_set_volume(voice_, soundGain(fVol));
}

void Channel::SetVolume(float volume)
{
	volume_ = volume;
	ChangeVolume(volume);
}

void Channel::UpdateVolume()
{
	SetVolume(volume_);
}

void Channel::SetPan(float pan)
{
	if(is3DSound_ || !voice_)
		return;

	// DirectSound attenuated the opposite channel along the same logarithmic curve as the
	// volume (CalcPan); miniaudio pans linearly across [-1, 1]. Only 2D sounds pan at all, and
	// the game centres almost all of them, so the curve is not worth carrying over.
	ma_sound_set_pan(voice_, clamp(2.0f*pan - 1.0f, -1.0f, 1.0f));
}

void Channel::SetPosition(const Vect3f& pos)
{
	position_ = pos;

	if(playing_ && is3DSound_)
		Apply3DParameters();
}

void Channel::GetPosition(Vect3f& pos)
{
	pos = position_;
}

void Channel::Set3DMinMaxDistance(float min, float max)
{
	minDistance_ = min;
	maxDistance_ = max;
	maxDistance2_ = max*max;
}

void Channel::Get3DMinMaxDistance(float* min, float* max)
{
	if(min)
		*min = minDistance_;
	if(max)
		*max = maxDistance_;
}

void Channel::Apply3DParameters()
{
	// TODO(sound-port): push position_ and the min/max distances at the voice's spatializer.
	// Voices are created unspatialized until the 3D slice, so there is nothing to push at.
}

Vect3f Channel::VectorToListener()
{
	// TODO(sound-port): there is no listener yet (SND3DListener is still a stub), so every
	// emitter reads as being on top of it.
	return Vect3f::ZERO;
}

bool Channel::isInFogOfWar()
{
	// TODO(sound-port): 3D-only, and it needs the emitter placed in the world first. The
	// original asked the active player's fog map about position_.
	return false;
}

void Channel::CheckFogOfWar()
{
	// TODO(sound-port): see isInFogOfWar().
}

//-----------------------------------------------------------------------------
// The SND* entry points the game calls (Sound.h). The DirectSound originals are in
// Sound/init.cpp.
//-----------------------------------------------------------------------------
bool SNDInitSound()
{
	if(!sndSystem.Init())
		return false;

	SNDEnableSound(true);
	return true;
}

void SNDReleaseSound()
{
	sndSystem.Release();
}

void SNDEnableSound(bool enable)
{
	sndSystem.EnableSound(enable);
}

bool SNDIsSoundEnabled()
{
	return sndSystem.IsEnabled();
}

void SNDSetVolume(float volume)
{
	sndSystem.SetGlobalVolume(volume);
}

float SNDGetVolume()
{
	return sndSystem.GetGlobalVolume();
}

void SNDSetFade(bool fadeIn, int time)
{
	sndSystem.StartFade(fadeIn, time);
}

void SNDSetGameActive(bool active)
{
	sndSystem.SetGameActive(active);
}

void SNDStopAll()
{
	sndSystem.StopAll();
}
