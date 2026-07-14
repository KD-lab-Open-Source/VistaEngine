#include "stdafx.h"
#include "UI_StreamVideo.h"

#include "Handle.h"
#include "GameOptions.h"
#include "SystemUtil.h"
#include "Video/VideoPlayer.h"
#include "Render/3dx/Umath.h"
#include "Render/inc/Unknown.h"
#include "Render/src/Texture.h"
#include "Render/src/TexLibrary.h"
#include "Render/src/VisGeneric.h"

// The Bink player, on ffmpeg (Video/VideoPlayer.cpp).
//
// RAD's binkw32 is what used to be here, and KD-lab stripped its calls out before releasing
// the source: what was left was the shape of a player -- a decode thread, two frame textures,
// a phase, a volume -- with nothing inside it, and an init() that reported success so the
// briefing screens' video-gated triggers would still run. The shape is gone with it. The
// decode thread in particular has no counterpart here: Bink needed to be pumped to keep its
// own audio fed, whereas VideoPlayer hands the soundtrack to miniaudio whole and pulls
// frames against the audio clock, on this thread, when quant() asks for them.
//
// Two things survive from the original in spirit:
//  - alphaPlan() still picks the blend mode, and still means "the video carries an alpha
//    plane". It used to read Bink's BINKALPHA flag (1<<20); it now asks the decoder, which
//    answers by choosing yuva420p over yuv420p. Same bit, from the other end.
//  - the frame is one texture, not two. The second one was the decode thread's half of a
//    double buffer, and without the thread it has nothing to do.

Singleton<UI_StreamVideo> streamVideo;

UI_StreamVideo::UI_StreamVideo()
: lock_()
, player_(0)
, texture_(0)
{
	release();
}

UI_StreamVideo::~UI_StreamVideo()
{
	release();
}

void UI_StreamVideo::release()
{
	MTAuto autoLock(lock_);

	delete player_;
	player_ = 0;

	RELEASE(texture_);

	fileName_.clear();
	started_ = false;
	needUpdate_ = false;
	cycle_ = false;
	mute_ = false;
}

bool UI_StreamVideo::init(const char* binkFileName, bool cycle)
{
	MTAuto autoLock(lock_);

	release();

	player_ = new VideoPlayer;
	if(!player_->open(binkFileName)){
		release();
		return false;
	}

	// The frame lands in a texture of exactly its own size -- no rounding up to a power of
	// two, which is why size() below is the whole texture. Bink demanded one and the original
	// paid for it with a partial UV rect; SDL GPU does not.
	texture_ = GetTexLibrary()->CreateTexture(player_->width(), player_->height(), true);
	if(!texture_){
		release();
		return false;
	}

	fileName_ = binkFileName;
	cycle_ = cycle;

	player_->setVolume(0.f);

	return true;
}

bool UI_StreamVideo::inited(const char* binkFileName) const
{
	return inited() && !stricmp(fileName_.c_str(), binkFileName);
}

void UI_StreamVideo::updateVolume()
{
	xassert(inited());

	if(!player_->paused() && !mute_ && GameOptions::instance().getBool(OPTION_VOICE_ENABLE) && applicationHasFocus())
		player_->setVolume(GameOptions::instance().getFloat(OPTION_VOICE_VOLUME));
	else
		player_->setVolume(0.f);
}

void UI_StreamVideo::updateTexture()
{
	if(!texture_)
		return;

	int pitch = 0;
	if(BYTE* dst = texture_->LockTexture(pitch)){
		player_->copyFrameTo(dst, pitch);
		texture_->UnlockTexture();
	}
}

void UI_StreamVideo::play()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	player_->play();
	started_ = true;

	updateVolume();

	// Put the first frame up now: the panel is drawn before the next quant() comes round, and
	// an empty texture would show as one frame of garbage.
	if(player_->quant())
		updateTexture();
}

void UI_StreamVideo::stop()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	started_ = false;

	player_->setVolume(0.f);
	player_->stop();
}

void UI_StreamVideo::phase(float newPhase)
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	player_->setPhase(clamp(newPhase, 0.f, 1.f));
}

float UI_StreamVideo::phase() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return 0.f;

	return player_->phase();
}

void UI_StreamVideo::pause(bool pause)
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	player_->pause(pause);
}

bool UI_StreamVideo::pause() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return false;

	return player_->paused();
}

bool UI_StreamVideo::grayScale() const
{
	// Bink's BINKGRAYSCALE (1<<17), which no video in the game sets: every .bik here decodes
	// to yuv420p or yuva420p, none to a luma-only format. It was never true, and it stays a
	// question the UI is entitled to ask.
	return false;
}

bool UI_StreamVideo::alphaPlan() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return false;

	return player_->hasAlpha();
}

void UI_StreamVideo::mute(bool muteOn)
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	if(muteOn != mute_){
		mute_ = muteOn;
		updateVolume();
	}
}

bool UI_StreamVideo::quant()
{
	MTAuto autoLock(lock_);

	if(!inited() || !started_)
		return false;

	updateVolume();

	if(player_->isEnd()){
		if(!cycle_){
			stop();
			return false;
		}

		player_->setPhase(0.f);
	}

	// The player decodes only when the clock says a new frame is due, so most quants upload
	// nothing: the video runs at 25 fps and the game does not.
	if(player_->quant())
		updateTexture();

	return true;
}

void UI_StreamVideo::setUpdated()
{
	MTAuto autoLock(lock_);
	needUpdate_ = true;
}

void UI_StreamVideo::ui_quant()
{
	MTAuto autoLock(lock_);

	if(needUpdate_){
		quant();
		needUpdate_ = false;
	}
	else if(started_)
		stop();
}

cTexture* UI_StreamVideo::texture() const
{
	MTAuto autoLock(lock_);

	return inited() ? texture_ : 0;
}

Vect2f UI_StreamVideo::size() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return Vect2f::ZERO;

	// The UV extent of the frame within its texture, and the texture is the frame.
	return Vect2f(1.f, 1.f);
}
