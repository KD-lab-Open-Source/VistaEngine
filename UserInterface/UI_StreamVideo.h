#ifndef __UI_BINK_VIDEO_H__
#define __UI_BINK_VIDEO_H__

class BinkSimplePlayer;
class cTexture;

class UI_StreamVideo
{
	mutable MTSection lock_;

public:
	UI_StreamVideo();
	~UI_StreamVideo();

	bool init(const char* binkFileName, bool cycle);
	bool inited(const char* binkFileName) const;
	void release();

	/// запустить/перезапустить проигрывание
	void play();
	void stop();
	
	void phase(float newPhase);
	float phase() const;

	void pause(bool pause);
	bool pause() const;

	bool grayScale() const;
	bool alphaPlan() const;

	void mute(bool muteOn);

	bool quant();
	
	void setUpdated();
	void ui_quant();
	
	/// текущий кадр
	cTexture* texture() const;
	Vect2f size() const;

private:
	bool inited() const { return player_; }
	void updateVolume();

	BinkSimplePlayer* player_;
	bool started_;
	bool needUpdate_;
	bool cycle_;
	bool mute_;
};

#endif //__UI_BINK_VIDEO_H__
