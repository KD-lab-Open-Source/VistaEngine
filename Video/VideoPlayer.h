#ifndef __VIDEO_PLAYER_H_INCLUDED__
#define __VIDEO_PLAYER_H_INCLUDED__

#include "VideoFile.h"

struct VideoSound;

/// Plays a .bik: the soundtrack through miniaudio, the picture pulled against it.
///
/// There is no decode thread. The original needed one because Bink's own API had to be pumped
/// to keep its audio fed; ours does not, because VideoFile hands the whole soundtrack to
/// miniaudio at open() and the audio device plays it without our help. That makes the audio
/// cursor the clock, and leaves the picture as something to pull on demand: quant() decodes
/// forward until the frame that is due, and the caller uploads it. Decoding 800x600 Bink costs
/// a couple of ms and happens once per displayed frame, on the thread that was going to block
/// on the GPU anyway.
///
/// A video with no soundtrack (three of them are) runs off the wall clock instead.
class VideoPlayer
{
public:
	VideoPlayer();
	~VideoPlayer();

	bool open(const char* fileName);
	void close();
	bool opened() const { return file_.opened(); }

	int width() const { return file_.width(); }
	int height() const { return file_.height(); }
	bool hasAlpha() const { return file_.hasAlpha(); }

	/// Start, or resume after pause().
	void play();
	/// Stop and rewind, the way the game's stop() means it.
	void stop();
	void pause(bool pause);
	bool paused() const { return paused_; }
	bool playing() const { return playing_; }

	/// 0..1
	void setVolume(float volume);
	float volume() const { return volume_; }

	/// Position through the video, 0..1. The game only ever *sets* 0 (the loop restart).
	float phase() const;
	void setPhase(float phase);

	/// Bring the picture up to the clock. True when a new frame was decoded and the caller
	/// has to re-upload frameBGRA(); false when the frame it is already showing still stands.
	bool quant();

	/// The video has run out. A looping caller answers this with setPhase(0).
	bool isEnd() const { return endOfStream_; }

	/// width * height * 4, B,G,R,A, top-down.
	const unsigned char* frameBGRA() const { return file_.frameBGRA(); }
	/// The same frame, copied row by row into a destination of the given pitch — which is what
	/// a locked cTexture is. Saves both callers from knowing the frame is tightly packed.
	void copyFrameTo(unsigned char* destination, int pitch) const;

private:
	VideoPlayer(const VideoPlayer&);
	VideoPlayer& operator=(const VideoPlayer&);

	/// Seconds into the video: the audio cursor when there is a soundtrack, the wall clock
	/// when there is not.
	double clock() const;
	void rewind();

	VideoFile file_;
	VideoSound* sound_;

	bool playing_;
	bool paused_;
	bool endOfStream_;
	bool frameShown_;
	float volume_;

	/// Wall-clock fallback, for a video with no soundtrack: when the current play started,
	/// and how far in we were at that point.
	double startTime_;
	double startPosition_;
};

#endif //__VIDEO_PLAYER_H_INCLUDED__
