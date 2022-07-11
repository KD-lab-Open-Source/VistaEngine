#include "StdAfx.h"
#include "UI_StreamVideo.h"

#include "Handle.h"
#include "GameOptions.h"
#include "SystemUtil.h"
#include "Render\3dx\Umath.h"
#include "Render\Inc\Unknown.h"
#include "Render\Src\Texture.h"
#include "Render\src\VisGeneric.h"

void* SNDGetDirectSound();

class BinkSimplePlayerImpl
{
public:
	BinkSimplePlayerImpl();
	~BinkSimplePlayerImpl();

	bool open();
	void stop();

	const char* getBinkFileName() const { return binkFile_.c_str(); }
	bool init(const char* bink_file);

	void setPhase(float phase);
	float getPhase() const { return 1.f; }

	void setPause(bool pause) { EBinkWait w; paused_ = pause; }
	bool getPause() const { return paused_; }

	void setVolume(float vol);
	float getVolume() const { return volume_; }

	cTexture* getTexture();

	bool isEnd() const { return true; }

	int flags() const { return 0; } 

private:
	int pBinkInfo_;

	class EBinkWait
	{
	public:
		EBinkWait()
		{
			if(waitEventHandle_ == INVALID_HANDLE_VALUE) 
				return;
			WaitForSingleObject(waitEventHandle_,INFINITE);
		}
		~EBinkWait()
		{
			if(waitEventHandle_ == INVALID_HANDLE_VALUE) 
				return;
			SetEvent(waitEventHandle_);
		}
	};

	void quant();
	static DWORD WINAPI threadProc(LPVOID lpParameter);
	void release();
	void computeFrame();
	void computeFrameQuant();

	static HANDLE waitEventHandle_;
	static HANDLE threadHandle_;
	static int threadStopFlag_;

	string binkFile_;

	cTexture* pTextureBink1_;
	cTexture* pTextureBink2_;

	float volume_;
	bool paused_;
};

HANDLE BinkSimplePlayerImpl::waitEventHandle_ = INVALID_HANDLE_VALUE;
HANDLE BinkSimplePlayerImpl::threadHandle_ = INVALID_HANDLE_VALUE;
int BinkSimplePlayerImpl::threadStopFlag_ = 0;

class BinkSimplePlayer
{
public:
	BinkSimplePlayer();
	~BinkSimplePlayer();

	static void preInit();

	bool open() { return player_->open(); }
	void stop() { player_->stop(); }
	
	bool init(const char* bink_file) { return player_->init(bink_file); }
	const char* getBinkFileName() const { return player_->getBinkFileName(); }

	void setPhase(float phase) { player_->setPhase(phase); }
	float getPhase() const { return player_->getPhase(); }

	void setPause(bool pause) { player_->setPause(pause); }
	bool getPause() const { return player_->getPause(); }

	void setVolume(float vol) { player_->setVolume(vol); }

	cTexture* getTexture() const { return player_->getTexture(); }

	bool isEnd() const { return player_->isEnd(); }

	int flags() const { return player_->flags(); } 

private:

	BinkSimplePlayerImpl* player_;
};

BinkSimplePlayer::BinkSimplePlayer()
{
	player_ = new BinkSimplePlayerImpl;
}

BinkSimplePlayer::~BinkSimplePlayer()
{
	delete player_;
}

void BinkSimplePlayer::preInit()
{
}

cTexture* BinkSimplePlayerImpl::getTexture() 
{ 
	//EBinkWait w; 
	return pTextureBink1_; 
}


DWORD WINAPI BinkSimplePlayerImpl::threadProc(LPVOID lpParameter)
{
	SetThreadPriority(threadHandle_,THREAD_PRIORITY_HIGHEST);

	BinkSimplePlayerImpl* pPlayer=(BinkSimplePlayerImpl*)lpParameter;

	float volume = pPlayer->getVolume();
	while(!threadStopFlag_)
	{
		{
			if(!pPlayer->getPause() && applicationHasFocus()){
				// если выключен звук
				if(!GameOptions::instance().getBool(OPTION_VOICE_ENABLE))
					pPlayer->setVolume(0.f);
				else
					if(volume != pPlayer->getVolume()){
						pPlayer->setVolume(GameOptions::instance().getFloat(OPTION_VOICE_VOLUME));
						volume = pPlayer->getVolume();
					}
				{
					EBinkWait w;
					pPlayer->quant();
				}
			}
			else 
				pPlayer->setVolume(0.f);
		}

		Sleep(1);
	}

	threadStopFlag_=2;
	return 0;
}

BinkSimplePlayerImpl::BinkSimplePlayerImpl()
{
	EBinkWait w;

	pTextureBink1_ = 0;
	pTextureBink2_ = 0;

	threadStopFlag_ = 0;
	paused_ = false;
}

void BinkSimplePlayerImpl::stop()
{
	if(threadHandle_ != INVALID_HANDLE_VALUE)
	{
		threadStopFlag_ = 1;
		while(threadStopFlag_ == 1)
			Sleep(10);
	}

	if(waitEventHandle_ != INVALID_HANDLE_VALUE)
		CloseHandle(waitEventHandle_);

	waitEventHandle_ = INVALID_HANDLE_VALUE;
	threadHandle_ = INVALID_HANDLE_VALUE;
}

BinkSimplePlayerImpl::~BinkSimplePlayerImpl()
{
	stop();
	release();
}

void BinkSimplePlayerImpl::release()
{
	if(pBinkInfo_)
	{
		try{
		} catch(...){}
		pBinkInfo_ = 0;
		binkFile_.clear();
	}

	RELEASE(pTextureBink1_);
	RELEASE(pTextureBink2_);
}

bool BinkSimplePlayerImpl::init(const char* bink_file) 
{ 
	pBinkInfo_ = 0;
	return false;

	EBinkWait w; 

	release();
	binkFile_ = bink_file; 

	try{
	}catch(...){
		pBinkInfo_ = 0;
		return false;
	}

	if(pTextureBink1_ == NULL || pTextureBink2_ == NULL){
		release();
		return false;
	}

	return true;
}

bool BinkSimplePlayerImpl::open()
{
	EBinkWait w;

	threadStopFlag_ = 2;

	if(threadHandle_ == INVALID_HANDLE_VALUE){
		threadStopFlag_ = 0;
		DWORD ThreadId;
		threadHandle_ = CreateThread(0, 0, threadProc, this, 0, &ThreadId);
		waitEventHandle_ = CreateEvent(0, FALSE, TRUE, 0);
	}

	return true;
}

void BinkSimplePlayerImpl::computeFrame()
{
	xassert(pBinkInfo_);
	int pitch = 0;
	pTextureBink2_->UnlockTexture();
}

void BinkSimplePlayerImpl::computeFrameQuant()
{
	xassert(pBinkInfo_);
	if(isEnd())
		return;
}
void BinkSimplePlayerImpl::setPhase(float phase)
{
	EBinkWait w;

	xassert(pBinkInfo_);
}

void BinkSimplePlayerImpl::setVolume(float vol)
{
	EBinkWait w;

	volume_ = vol;
	xassert(pBinkInfo_);
	const float amplification = 0.8f;

	if(vol < amplification)
		vol = vol / amplification * 32768;
	else
		vol = 32767 + (vol - amplification) / (1.f - amplification) * 32768;

}

void BinkSimplePlayerImpl::quant()
{
	xassert(pBinkInfo_);
	computeFrameQuant();
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
	
	if(!player_->init(binkFileName))
	{
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
	if(!player_->getPause() && GameOptions::instance().getBool(OPTION_VOICE_ENABLE) && applicationHasFocus())
		if(!mute_)
			player_->setVolume(GameOptions::instance().getFloat(OPTION_VOICE_VOLUME));
		else
			player_->setVolume(0);
	else
		player_->setVolume(0);
}

void UI_StreamVideo::play()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	if(!player_->open()){
		release();
		return;
	}

	started_ = true;

	updateVolume();
}

void UI_StreamVideo::stop()
{
	MTAuto autoLock(lock_);

	if(!inited())
		return;

	started_ = false;

	player_->setVolume(0);
	player_->setPhase(0.f);
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

	if(player_->isEnd())
		if(cycle_)
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
		return Vect2f::ZERO;

	return Vect2f::ZERO;
}
