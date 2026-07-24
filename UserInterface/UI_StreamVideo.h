#ifndef __UI_BINK_VIDEO_H__
#define __UI_BINK_VIDEO_H__

class VideoPlayer;
class cTexture;

/// The video panel: one video at a time, shared by every UI_ControlVideo on screen (there is
/// never more than one playing). The briefing screens are what this is for.
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

	/// The window gained or lost focus. Unfocused, the main loop stops calling quant() and the
	/// picture stops; the soundtrack, which miniaudio plays on its own thread, does not, and has
	/// to be stopped with it or the two come back out of step. Composed with pause() rather than
	/// overwriting it -- the original's decode thread gated on `!getPause() && applicationHasFocus()`,
	/// two independent conditions, and never wrote one from the other.
	void setApplicationActive(bool active);

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
	bool inited() const { return player_ != 0; }
	void updateVolume();
	/// Copy the frame the player is holding into the texture.
	void updateTexture();
	/// Push paused_ and applicationActive_ down to the player as the one pause it understands.
	void applyPause();

	VideoPlayer* player_;
	cTexture* texture_;
	string fileName_;

	bool started_;
	bool needUpdate_;
	bool cycle_;
	bool mute_;
	/// The pause the UI asked for, kept apart from the one focus imposes.
	bool paused_;
	bool applicationActive_;
};

#endif //__UI_BINK_VIDEO_H__
