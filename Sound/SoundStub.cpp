// The last silent corner of the Sound module: OggPlayer.
//
// The device, the sample pool, its voices and the 3D listener are all real and miniaudio's
// (AudioBackend.cpp, SoundSystemMiniaudio.cpp). What is left is the music and voice streaming,
// which needs a Vorbis decoder — miniaudio has none built in. That is the next slice. The
// original is XLibs.Net/OGG/PlayOgg, still in the tree, as the reference.
#include "StdAfx.h"
#include "PlayOgg.h"

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
