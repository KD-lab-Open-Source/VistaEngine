// The not-yet-ported half of the Sound module: no-op OggPlayer and SND3DListener.
//
// The device, the sample pool and its voices are real and miniaudio's (AudioBackend.cpp,
// SoundSystemMiniaudio.cpp). What is left silent here is the music and voice streaming
// (OggPlayer) and the 3D listener (SND3DListener) — so effects are audible, but flat, and
// nothing plays music. Each lands in its own slice, and the original of each is still in the
// tree as the reference: C3D.cpp / init.cpp (SND3DListener) and XLibs.Net/OGG/PlayOgg.
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
