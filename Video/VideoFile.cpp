#include "VideoFile.h"

#include "XZip.h"

#include <stdio.h>
#include <string.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

// The ffmpeg build behind this file carries two demuxers and three decoders and nothing else
// — see cmake/FFmpeg.cmake for the configure line and why each component is there.

namespace {

// ---------------------------------------------------------------------------
// The VFS
// ---------------------------------------------------------------------------
// ffmpeg is built with no protocols at all, so there is no avformat_open_input(path) to fall
// back on: every open goes through an AVIOContext, and this is it. XZipStream resolves a name
// against the mounted .pak archives first and the filesystem second, which is what puts the
// videos in reach on the same terms as every other asset. (The .bik and .avi files happen to
// be loose on disk today; routing them through the VFS is what stops that from being load-
// bearing.) The mirror of Sound/AudioBackend.cpp's ZipVFS, one layer down.
//
// No lock here, unlike that one: this decodes on the game thread, alongside every other asset
// load, while miniaudio reads on a job thread of its own and has to serialize against itself.

int vfsRead(void* opaque, uint8_t* buf, int size)
{
	unsigned long read = static_cast<XZipStream*>(opaque)->read(buf, (unsigned long)size);
	return read ? (int)read : AVERROR_EOF;
}

int64_t vfsSeek(void* opaque, int64_t offset, int whence)
{
	XZipStream* stream = static_cast<XZipStream*>(opaque);

	// avformat asks for the size through the seek callback rather than a separate one.
	if(whence == AVSEEK_SIZE)
		return stream->size();

	int dir;
	switch(whence){
	case SEEK_SET: dir = XZS_BEG; break;
	case SEEK_CUR: dir = XZS_CUR; break;
	case SEEK_END: dir = XZS_END; break;
	default: return -1;
	}

	stream->seek((long)offset, dir);
	return stream->tell();
}

const int VFS_BUFFER_SIZE = 32 * 1024;

void initFFmpeg()
{
	static bool done = false;
	if(done)
		return;
	done = true;

	// Quiet by default: swscale announces "no accelerated colorspace conversion found" for
	// every context it builds (we compile without x86 asm, on purpose), and that is not news.
	av_log_set_level(AV_LOG_ERROR);
}

}

// ---------------------------------------------------------------------------

struct VideoFileImpl
{
	XZipStream* stream;
	AVIOContext* avio;
	AVFormatContext* format;

	AVCodecContext* video;
	AVCodecContext* audio;
	int videoStream;
	int audioStream;

	SwsContext* sws;
	AVPacket* packet;
	AVFrame* frame;

	VideoFileImpl()
	: stream(0), avio(0), format(0), video(0), audio(0)
	, videoStream(-1), audioStream(-1), sws(0), packet(0), frame(0)
	{}
};

VideoFile::VideoFile()
: impl_(0)
, width_(0), height_(0), hasAlpha_(false)
, duration_(0.f), frameRate_(0.f), frameCount_(0)
, framePosition_(0.0), endOfStream_(false)
, audioSampleRate_(0), audioChannels_(0)
{
}

VideoFile::~VideoFile()
{
	close();
}

bool VideoFile::open(const char* fileName)
{
	close();
	initFFmpeg();

	VideoFileImpl* impl = new VideoFileImpl;

	// handleErrors = false: a missing video has to come back as a return code, not as the
	// archive layer's own error box.
	impl->stream = new XZipStream(false);
	if(!impl->stream->open(fileName, XZS_IN)){
		fprintf(stderr, "video: cannot open %s\n", fileName);
		delete impl->stream;
		delete impl;
		return false;
	}

	unsigned char* buffer = (unsigned char*)av_malloc(VFS_BUFFER_SIZE);
	impl->avio = avio_alloc_context(buffer, VFS_BUFFER_SIZE, 0, impl->stream, vfsRead, 0, vfsSeek);
	impl->format = avformat_alloc_context();
	impl->format->pb = impl->avio;

	if(avformat_open_input(&impl->format, 0, 0, 0) < 0){
		fprintf(stderr, "video: %s is not a video we can decode\n", fileName);
		// avformat_open_input freed the format context on failure but not our AVIO.
		av_freep(&impl->avio->buffer);
		avio_context_free(&impl->avio);
		delete impl->stream;
		delete impl;
		return false;
	}

	impl_ = impl;   // from here on close() owns the cleanup

	if(avformat_find_stream_info(impl->format, 0) < 0){
		fprintf(stderr, "video: no stream info in %s\n", fileName);
		close();
		return false;
	}

	impl->videoStream = av_find_best_stream(impl->format, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
	impl->audioStream = av_find_best_stream(impl->format, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);

	if(impl->videoStream < 0){
		fprintf(stderr, "video: no video stream in %s\n", fileName);
		close();
		return false;
	}

	AVStream* stream = impl->format->streams[impl->videoStream];
	const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if(!codec){
		fprintf(stderr, "video: %s wants a decoder that is not in this ffmpeg build\n", fileName);
		close();
		return false;
	}

	impl->video = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(impl->video, stream->codecpar);
	if(avcodec_open2(impl->video, codec, 0) < 0){
		fprintf(stderr, "video: cannot open the %s decoder for %s\n", codec->name, fileName);
		close();
		return false;
	}

	width_ = impl->video->width;
	height_ = impl->video->height;
	// Bink carries its alpha as a fourth plane, and the decoder says so by choosing a pixel
	// format that has one. That is the same bit the original read out of the BINK struct
	// (BINKALPHA, 1<<20) to decide the blend mode, arrived at from the other end.
	hasAlpha_ = impl->video->pix_fmt == AV_PIX_FMT_YUVA420P || impl->video->pix_fmt == AV_PIX_FMT_BGRA;

	AVRational rate = stream->avg_frame_rate;
	frameRate_ = rate.den ? (float)av_q2d(rate) : 0.f;
	// .avi counts its frames; .bik does not, and reports a duration instead.
	frameCount_ = (int)stream->nb_frames;
	duration_ = impl->format->duration != AV_NOPTS_VALUE
	          ? (float)(impl->format->duration / (double)AV_TIME_BASE)
	          : 0.f;
	if(!frameCount_ && frameRate_ > 0.f)
		frameCount_ = (int)(duration_ * frameRate_ + 0.5f);

	if(width_ <= 0 || height_ <= 0){
		fprintf(stderr, "video: %s has no size\n", fileName);
		close();
		return false;
	}

	// Everything the game draws is BGRA: yuv420p and yuva420p (Bink) and the bottom-up bgra of
	// a raw-DIB .avi all land in the same buffer through this. The .avi's negative linesize is
	// what flips it the right way up on the way, which is the job the VFW path did by hand.
	impl->sws = sws_getContext(width_, height_, impl->video->pix_fmt,
	                           width_, height_, AV_PIX_FMT_BGRA,
	                           SWS_BILINEAR, 0, 0, 0);
	if(!impl->sws){
		fprintf(stderr, "video: no conversion from %s to BGRA for %s\n",
		        av_get_pix_fmt_name(impl->video->pix_fmt), fileName);
		close();
		return false;
	}

	impl->packet = av_packet_alloc();
	impl->frame = av_frame_alloc();
	frame_.assign((size_t)width_ * height_ * 4, 0);

	if(impl->audioStream >= 0){
		AVStream* audioStream = impl->format->streams[impl->audioStream];
		const AVCodec* audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
		if(audioCodec){
			impl->audio = avcodec_alloc_context3(audioCodec);
			avcodec_parameters_to_context(impl->audio, audioStream->codecpar);
			if(avcodec_open2(impl->audio, audioCodec, 0) >= 0){
				audioSampleRate_ = impl->audio->sample_rate;
				audioChannels_ = impl->audio->ch_layout.nb_channels;
				decodeAudioTrack();
			}
			else{
				// Playable without its soundtrack, so this is not fatal.
				fprintf(stderr, "video: cannot open the %s decoder for %s\n", audioCodec->name, fileName);
				avcodec_free_context(&impl->audio);
			}
		}
	}

	return true;
}

void VideoFile::close()
{
	if(impl_){
		if(impl_->sws) sws_freeContext(impl_->sws);
		if(impl_->frame) av_frame_free(&impl_->frame);
		if(impl_->packet) av_packet_free(&impl_->packet);
		if(impl_->video) avcodec_free_context(&impl_->video);
		if(impl_->audio) avcodec_free_context(&impl_->audio);

		// The AVIO is ours, not avformat's (AVFMT_FLAG_CUSTOM_IO), so closing the input
		// leaves it — and the buffer ffmpeg reallocated inside it — for us to free.
		if(impl_->format) avformat_close_input(&impl_->format);
		if(impl_->avio){
			av_freep(&impl_->avio->buffer);
			avio_context_free(&impl_->avio);
		}
		delete impl_->stream;

		delete impl_;
		impl_ = 0;
	}

	width_ = height_ = 0;
	hasAlpha_ = false;
	duration_ = frameRate_ = 0.f;
	frameCount_ = 0;
	frame_.clear();
	framePosition_ = 0.0;
	endOfStream_ = false;
	audio_.clear();
	audioSampleRate_ = audioChannels_ = 0;
}

void VideoFile::decodeAudioTrack()
{
	VideoFileImpl* impl = impl_;

	// Every soundtrack in the game is binkaudio_dct, which decodes to planar float. Nothing
	// else can turn up here — the ffmpeg build has no other audio decoder in it — so this is
	// the one layout to unpack, rather than a reason to link swresample.
	if(impl->audio->sample_fmt != AV_SAMPLE_FMT_FLTP){
		fprintf(stderr, "video: unexpected audio sample format %s\n",
		        av_get_sample_fmt_name(impl->audio->sample_fmt));
		audioSampleRate_ = 0;
		return;
	}

	const int channels = audioChannels_;
	if(channels <= 0){
		audioSampleRate_ = 0;
		return;
	}

	// A guess at the final size, so the appends below do not walk the vector up from nothing.
	if(duration_ > 0.f)
		audio_.reserve((size_t)(duration_ * audioSampleRate_) * channels);

	while(av_read_frame(impl->format, impl->packet) >= 0){
		if(impl->packet->stream_index == impl->audioStream &&
		   avcodec_send_packet(impl->audio, impl->packet) >= 0){
			while(avcodec_receive_frame(impl->audio, impl->frame) >= 0){
				const int samples = impl->frame->nb_samples;
				const size_t base = audio_.size();
				audio_.resize(base + (size_t)samples * channels);

				for(int c = 0; c < channels; ++c){
					const float* src = (const float*)impl->frame->data[c];
					short* dst = &audio_[base] + c;
					for(int i = 0; i < samples; ++i, dst += channels){
						float sample = src[i] * 32767.f;
						*dst = sample >= 32767.f ? 32767 : (sample <= -32768.f ? -32768 : (short)sample);
					}
				}
			}
		}
		av_packet_unref(impl->packet);
	}

	avcodec_flush_buffers(impl->audio);
	rewind();
}

bool VideoFile::decodeFrame()
{
	if(!impl_ || endOfStream_)
		return false;

	VideoFileImpl* impl = impl_;

	for(;;){
		// Anything already sitting in the decoder comes out first.
		int result = avcodec_receive_frame(impl->video, impl->frame);

		if(result == 0){
			unsigned char* dst = &frame_[0];
			const int pitch = width_ * 4;
			sws_scale(impl->sws, impl->frame->data, impl->frame->linesize, 0, height_, &dst, &pitch);

			// Bink numbers its frames in a 1/fps time base, so a pts is a frame index there;
			// either way this is seconds. A file that carries no pts falls back on counting.
			framePosition_ = impl->frame->pts != AV_NOPTS_VALUE
			               ? impl->frame->pts * av_q2d(impl->format->streams[impl->videoStream]->time_base)
			               : framePosition_ + frameTime();
			return true;
		}

		if(result != AVERROR(EAGAIN)){    // AVERROR_EOF, or a decode that went wrong
			endOfStream_ = true;
			return false;
		}

		// The decoder wants another packet. Video ones go in; the audio was consumed whole
		// at open() and its packets are dropped on the floor here.
		for(;;){
			if(av_read_frame(impl->format, impl->packet) < 0){
				// End of file: drain whatever the decoder is still holding.
				avcodec_send_packet(impl->video, 0);
				break;
			}

			const bool isVideo = impl->packet->stream_index == impl->videoStream;
			if(isVideo)
				avcodec_send_packet(impl->video, impl->packet);
			av_packet_unref(impl->packet);

			if(isVideo)
				break;
		}
	}
}

void VideoFile::seek(double seconds)
{
	if(!impl_)
		return;

	AVStream* stream = impl_->format->streams[impl_->videoStream];
	const int64_t timestamp = (int64_t)(seconds / av_q2d(stream->time_base));

	// BACKWARD: land on the keyframe at or before the target, because a Bink frame is a delta
	// against the one before it and decoding from anywhere else produces garbage.
	av_seek_frame(impl_->format, impl_->videoStream, timestamp, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(impl_->video);

	// Provisional: the next decoded frame carries the position it actually landed on.
	framePosition_ = seconds;
	endOfStream_ = false;
}
