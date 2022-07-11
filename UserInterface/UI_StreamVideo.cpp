#include "StdAfx.h"
#include "UI_StreamVideo.h"

#include "Handle.h"
#include "GameOptions.h"

void* SNDGetDirectSound();

class BinkSimplePlayer
{
public:
	BinkSimplePlayer();
	~BinkSimplePlayer();

	static void preInit();

	bool open(const char* bink_file);
	void quant();
	
	const char* getBinkFileName() const { return binkFile_.c_str(); }

	void setPhase(float phase);
	float getPhase() const { return 1.f; }

	void setPause(bool pause) {  }
	bool getPause() const { return false; }

	void setVolume(float vol);

	cTexture* getTexture() const { return pTextureBink_; }
	Vect2i getSize() const { return Vect2i(0, 0); }

	bool isEnd() const { return true; }

	int flags() const { return 0; } 

private:
	void release();
	void computeFrame();

	string binkFile_;
	cTexture* pTextureBink_;
};

void BinkSimplePlayer::preInit()
{
}

BinkSimplePlayer::BinkSimplePlayer()
{
	pTextureBink_ = 0;
}

BinkSimplePlayer::~BinkSimplePlayer()
{
	release();
}

void BinkSimplePlayer::release()
{

	RELEASE(pTextureBink_);
}

bool BinkSimplePlayer::open(const char* bink_file)
{
	release();
	
		return false;

	return true;
}

void BinkSimplePlayer::computeFrame()
{
	if(isEnd())
		return;

	int pitch = 0;

	pTextureBink_->UnlockTexture();
	
}

void BinkSimplePlayer::setPhase(float phase)
{
	computeFrame();
}

void BinkSimplePlayer::setVolume(float vol)
{
	const float amplification = 0.8f;

	if(vol < amplification)
		vol = vol / amplification * 32768;
	else
		vol = 32767 + (vol - amplification) / (1.f - amplification) * 32768;

}

void BinkSimplePlayer::quant()
{
	computeFrame();
}


// -------------- UI_StreamVideo()

Singleton<UI_StreamVideo> streamVideo;


UI_StreamVideo::UI_StreamVideo()
: lock_()
, player_(0)
{
	release();
	BinkSimplePlayer::preInit();
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
	started_ = false;
	needUpdate_ = false;
	cycle_ = false;
	mute_ = false;
}

bool UI_StreamVideo::init(const char* binkFileName, bool cycle)
{
	MTAuto autoLock(lock_);

	release();
	player_ = new BinkSimplePlayer;

	if(!player_->open(binkFileName)){
		release();
		return false;
	}
	
	player_->setVolume(0);

	cycle_ = cycle;
	
	return true;
}

bool UI_StreamVideo::inited(const char* binkFileName) const
{
	return inited() && !stricmp(player_->getBinkFileName(), binkFileName);
}

void UI_StreamVideo::updateVolume()
{
	xassert(inited());
	
	if(!mute_)
		player_->setVolume(GameOptions::instance().getFloat(OPTION_VOICE_VOLUME));
	else
		player_->setVolume(0);
}

void UI_StreamVideo::play()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	started_ = true;

	updateVolume();
	player_->setPhase(0.f);
}

void UI_StreamVideo::stop()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	started_ = false;

	player_->setVolume(0);
	player_->setPhase(0.f);
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

	return player_->getPhase();
}

void UI_StreamVideo::pause(bool pause)
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	player_->setPause(pause);
}

bool UI_StreamVideo::pause() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return false;

	return player_->getPause();
}

bool UI_StreamVideo::grayScale() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return false;

	return player_->flags() & (1 << 17);
}

bool UI_StreamVideo::alphaPlan() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return false;

	return player_->flags() & (1 << 20);
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

	if(!player_->isEnd())
		player_->quant();
	else if(cycle_)
		player_->setPhase(0.f);
	else{
		stop();
		return false;
	}

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

	if(inited())
		return player_->getTexture();
	return 0;
}

Vect2f UI_StreamVideo::size() const
{
	MTAuto autoLock(lock_);

	if(!inited())
		return Vect2f::ZERO;;

	if(cTexture* tx = texture()){
		Vect2i binkSize = player_->getSize();
		return Vect2f(float(binkSize.x) / float(tx->GetWidth()), float(binkSize.y) / float(tx->GetHeight()));
	}

	return Vect2f::ZERO;
}
