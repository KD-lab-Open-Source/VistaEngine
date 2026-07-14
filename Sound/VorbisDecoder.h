#ifndef __VORBIS_DECODER_H_INCLUDED__
#define __VORBIS_DECODER_H_INCLUDED__

#include "miniaudio.h"

/// miniaudio decodes wav, flac and mp3 out of the box — but not Vorbis, and every piece of music
/// and every line of speech in the game is a .ogg. This is a Vorbis decoding backend for it, built
/// on stb_vorbis, registered with the resource manager in Sound/AudioBackend.cpp.
ma_decoding_backend_vtable* vorbisDecodingBackend();

#endif
