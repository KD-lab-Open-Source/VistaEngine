// Non-Windows stub for the Sound module: no-op (silent) audio.
//
// The real backend is DirectSound-based (SoundSystem.cpp / WaveFile.cpp / ...)
// and stays WIN32-only. These stubs provide real C++ definitions (correct
// signatures, no-op bodies) for the SoundSystem / Sound / Channel / OggPlayer
// symbols the engine references, so the game links and runs silently until the
// SDL3 audio backend (Track B) replaces them. See Sound/CMakeLists.txt.
#include "StdAfx.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "PlayOgg.h"

// ---- free functions (Sound.h) ----
bool SNDInitSound(HWND, bool, bool) { return false; }
void SNDReleaseSound() {}
void SNDEnableSound(bool) {}
bool SNDIsSoundEnabled() { return false; }
void SNDSetVolume(float) {}
void SNDSetFade(bool, int) {}
void SNDSetGameActive(bool) {}
void SNDStopAll() {}

// ---- globals ----
SoundSystem sndSystem;
SND3DListener snd_listener;

// ---- SND3DListener (Sound.h) ----
SND3DListener::SND3DListener() {}
SND3DListener::~SND3DListener() {}
bool SND3DListener::SetPos(const MatXf&) { return false; }
bool SND3DListener::SetVelocity(const Vect3f&) { return false; }
bool SND3DListener::Update() { return false; }

// ---- SoundSystem (SoundSystem.h) ----
SoundSystem::SoundSystem() {}
SoundSystem::~SoundSystem() {}
Sound* SoundSystem::CreateSound(const char*, DWORD) { return 0; }
void SoundSystem::Update() {}
int SoundSystem::numberOfPlayingSounds() { return 0; }
int SoundSystem::numberOfUsedSounds() { return 0; }
void SoundSystem::EnableSound(bool) {}
void SoundSystem::RecalculateClipDistance() {}
void SoundSystem::Mute3DSounds(bool) {}
void SoundSystem::StartFade(bool, int, bool) {}
void SoundSystem::SetStandbyTime(float) {}

// ---- Sound (SoundSystem.h) ----
Channel* Sound::CreateAndPlayChannel(bool) { return 0; }
bool Sound::PlaySound(const Vect3f&) { return false; }
void Sound::Set3DMinMaxDistance(float, float) {}
void Sound::SetMaxChannels(int) {}
void Sound::SetVolume(float) {}

// ---- Channel (SoundSystem.h) ----
void Channel::Play() {}
void Channel::Stop(bool) {}
void Channel::SetLoop(bool) {}
void Channel::Release() {}
void Channel::SetPosition(const Vect3f&) {}
void Channel::SetVolume(float) {}
void Channel::SetPan(float) {}
bool Channel::IsPlaying() { return false; }
void Channel::SetMute(bool) {}

// ---- OggPlayer (PlayOgg.h) ----
OggPlayer::OggPlayer() : player_(0) {}
OggPlayer::~OggPlayer() {}
bool OggPlayer::play(const char*, bool, void*) { return false; }
void OggPlayer::stop() {}
void OggPlayer::pause() {}
void OggPlayer::resume() {}
OggState OggPlayer::state() const { return OGG_STOPPED; }
void OggPlayer::setVolume(int) {}
bool OggPlayer::fadeVolume(float, float) { return false; }
void OggPlayer::setCallbacks(OggCallbacks*) {}
double OggPlayer::getLength(const char*) { return 0.0; }
bool OggPlayer::initLibrary(void*) { return false; }
void OggPlayer::finitLibrary() {}
