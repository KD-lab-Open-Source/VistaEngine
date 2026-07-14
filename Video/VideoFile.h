#ifndef __VIDEO_FILE_H_INCLUDED__
#define __VIDEO_FILE_H_INCLUDED__

#include <vector>

struct VideoFileImpl;

/// One video file, decoded through ffmpeg: a Bink 1 movie (.bik) or one of the animated
/// textures the game ships as raw-DIB .avi. It reads through the game's VFS, so a name is
/// the same name the scripts use ("Resource\\Video\\P2_intro.bik") and an asset inside a
/// .pak is as reachable as a loose one.
///
/// Frames come out as BGRA, top-down, which is the byte order cTexture stages and uploads —
/// the caller can memcpy a frame straight into a locked texture.
///
/// The soundtrack, if the file has one, is decoded *whole* by open() into interleaved 16-bit
/// PCM. That is what lets the player above run without a decode thread: with the audio already
/// in memory it can be handed to miniaudio in one piece, the audio device becomes the clock,
/// and video frames are pulled on demand against it. The cost is bounded and small — the
/// longest video in the game is the 132-second intro, whose track is 23 MB.
///
/// Not thread-safe, and not meant to be: like every other asset load in the engine, this runs
/// on the game thread.
class VideoFile
{
public:
	VideoFile();
	~VideoFile();

	bool open(const char* fileName);
	void close();
	bool opened() const { return impl_ != 0; }

	int width() const { return width_; }
	int height() const { return height_; }

	/// Bink's alpha plane (the file decodes to yuva420p rather than yuv420p). Every briefing
	/// video has one and every full-screen one does not; the UI keys its blend mode on it.
	bool hasAlpha() const { return hasAlpha_; }

	/// Seconds. 0 if the container does not say.
	float duration() const { return duration_; }
	float frameRate() const { return frameRate_; }
	/// Seconds per frame, 0 if the frame rate is unknown.
	float frameTime() const { return frameRate_ > 0.f ? 1.f / frameRate_ : 0.f; }
	/// Frames, when the container counts them: .avi does, .bik does not (0 there).
	int frameCount() const { return frameCount_; }

	/// Decode the next frame. False at end of stream, and once it has said so the frame
	/// buffer keeps the last good frame rather than going blank.
	bool decodeFrame();
	/// width * height * 4 bytes, B,G,R,A, top-down. Null until the first decodeFrame().
	const unsigned char* frameBGRA() const { return frame_.empty() ? 0 : &frame_[0]; }
	/// Presentation time of the frame decodeFrame() last produced, in seconds.
	double framePosition() const { return framePosition_; }

	/// Back to the first frame. The soundtrack is untouched — it is already in memory.
	void rewind() { seek(0.0); }
	/// To the last frame at or before `seconds`. The picture resumes from there.
	void seek(double seconds);

	// The soundtrack, decoded at open(): interleaved 16-bit samples, or empty.
	bool hasAudio() const { return audioSampleRate_ > 0 && !audio_.empty(); }
	int audioSampleRate() const { return audioSampleRate_; }
	int audioChannels() const { return audioChannels_; }
	const short* audioSamples() const { return audio_.empty() ? 0 : &audio_[0]; }
	/// Sample frames, i.e. samples per channel.
	size_t audioFrames() const { return audioChannels_ ? audio_.size() / audioChannels_ : 0; }

private:
	VideoFile(const VideoFile&);
	VideoFile& operator=(const VideoFile&);

	/// Demux the whole file once, decoding only the audio packets. Leaves the file rewound.
	void decodeAudioTrack();

	VideoFileImpl* impl_;

	int width_;
	int height_;
	bool hasAlpha_;
	float duration_;
	float frameRate_;
	int frameCount_;

	std::vector<unsigned char> frame_;
	double framePosition_;
	bool endOfStream_;

	std::vector<short> audio_;
	int audioSampleRate_;
	int audioChannels_;
};

#endif //__VIDEO_FILE_H_INCLUDED__
