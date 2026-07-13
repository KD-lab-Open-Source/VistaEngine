// SoundSystem on miniaudio — the device half.
//
// The DirectSound original is still in the tree, unbuilt, as the reference for the rest of
// the port: SoundSystem.cpp (this class), WaveFile.cpp, C3D.cpp, SoftwareBuffer.cpp,
// HardwareBuffer.cpp, init.cpp (the SND* functions below). Read it before porting a
// behaviour rather than inventing one.
//
// What is real here: the device, the VFS, the mix buses, and the global volume / enable /
// focus state that rides on them. Sound and Channel — the sample pool and the voices — are
// still the no-ops in SoundStub.cpp, so nothing is audible yet; they land with the next
// slice. Sites that wait on them are tagged TODO(sound-port).
#include "StdAfx.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "AudioBackend.h"

SoundSystem sndSystem;

SoundSystem::SoundSystem()
{
	globalVolume_ = 1.0f;
	globalFadeFactor_ = 1.0f;
	enable_ = false;
	mute3Dsounds_ = false;
	gameActive_ = true;
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

	audio::setGroupVolume(audio::GROUP_SFX, globalVolume_);
	ApplyMuteState();

	return true;
}

void SoundSystem::Release()
{
	audio::release();
}

void* SoundSystem::GetAudioEngine()
{
	return audio::engine();
}

Sound* SoundSystem::CreateSound(const char* filename, DWORD mode)
{
	return 0;   // TODO(sound-port): the sample pool lands with the SFX slice
}

void SoundSystem::Update()
{
}

// The engine mutes on two independent counts — the sound option being off, and the window
// losing focus — and both have to be able to hold the mute on their own.
void SoundSystem::ApplyMuteState()
{
	audio::setMuted(!enable_ || !gameActive_);
}

void SoundSystem::EnableSound(bool enable)
{
	enable_ = enable;
	ApplyMuteState();
}

bool SoundSystem::IsEnabled()
{
	return enable_;
}

void SoundSystem::SetGameActive(bool active)
{
	gameActive_ = active;
	ApplyMuteState();
}

void SoundSystem::SetGlobalVolume(float volume)
{
	globalVolume_ = clamp(volume, 0.0f, 1.0f);
	audio::setGroupVolume(audio::GROUP_SFX, globalVolume_);
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
}

void SoundSystem::MuteAll(bool mute)
{
}

void SoundSystem::Mute3DSounds(bool mute)
{
	mute3Dsounds_ = mute;   // TODO(sound-port): acted on per-channel by the 3D slice
}

void SoundSystem::RecalculateClipDistance()
{
}

void SoundSystem::StartFade(bool fadeIn, int time, bool allSounds)
{
	// Recorded, not acted on: a fade is per-channel work (Channel::useGlobalFade_ lets a
	// sound opt out of it), and there are no channels yet.
	// TODO(sound-port): drive the fade from the SFX slice.
	needFade_ = true;
	globalFadeIn = fadeIn;
	fadeTime = (float)time;
	fadeAllSounds_ = allSounds;
}

void SoundSystem::SetStandbyTime(float time)
{
	standbyTime_ = time;
}

//-----------------------------------------------------------------------------
// The SND* entry points the game calls (Sound.h). The DirectSound originals are in
// Sound/init.cpp.
//-----------------------------------------------------------------------------
bool SNDInitSound()
{
	return sndSystem.Init();
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
