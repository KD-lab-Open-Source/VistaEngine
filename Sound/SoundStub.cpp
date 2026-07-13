// The not-yet-ported half of the Sound module: no-op (silent) Sound, Channel, OggPlayer
// and SND3DListener.
//
// The device itself is real and is miniaudio's (AudioBackend.cpp, SoundSystemMiniaudio.cpp).
// What is still stubbed is everything that would make a noise through it — the sample pool
// and its voices, the music/voice streams, and the 3D listener — so the game runs silent.
// Each lands in its own slice, and the DirectSound original of each is still in the tree as
// the reference: SoundSystem.cpp (Sound, Channel), C3D.cpp (SND3DListener), and
// XLibs.Net/OGG/PlayOgg (OggPlayer).
#include "StdAfx.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "PlayOgg.h"

// ---- globals ----
SND3DListener snd_listener;

// ---- SND3DListener (Sound.h) ----
SND3DListener::SND3DListener() {}
SND3DListener::~SND3DListener() {}
bool SND3DListener::SetPos(const MatXf&) { return false; }
bool SND3DListener::SetVelocity(const Vect3f&) { return false; }
bool SND3DListener::Update() { return false; }

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
