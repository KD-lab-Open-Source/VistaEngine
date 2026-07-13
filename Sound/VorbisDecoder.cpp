#include "StdAfx.h"
#include "VorbisDecoder.h"

extern "C" {
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
}

namespace {

/// A Vorbis stream, decoded by stb_vorbis, dressed as a miniaudio data source.
///
/// stb_vorbis cannot read through callbacks: it decodes from a filename, a FILE*, or a block of
/// memory, and we have none of those to give it. The audio arrives through the engine's own VFS
/// (Sound/AudioBackend.cpp) — for the speech, from inside a .pak — so the compressed file is pulled
/// into memory once, here, and decoded from there.
///
/// That is less extravagant than it sounds. A music track is about 4 MB compressed against roughly
/// 80 MB of PCM, and it is the PCM that matters: it is still decoded a page at a time as the track
/// plays, rather than up front. Only the one playing track and the one line of speech are ever
/// resident.
struct VorbisDecoder
{
	ma_data_source_base ds;   // must be first: miniaudio casts ma_data_source* to this
	stb_vorbis* vorbis;
	unsigned char* fileData;
	ma_uint32 channels;
	ma_uint32 sampleRate;
	ma_allocation_callbacks allocationCallbacks;
};

ma_result decoderRead(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pDataSource;

	*pFramesRead = 0;
	if(frameCount == 0)
		return MA_SUCCESS;

	int framesRead = stb_vorbis_get_samples_float_interleaved(decoder->vorbis, (int)decoder->channels,
	                                                          (float*)pFramesOut,
	                                                          (int)(frameCount*decoder->channels));
	if(framesRead < 0)
		return MA_ERROR;

	*pFramesRead = (ma_uint64)framesRead;

	return (framesRead == 0) ? MA_AT_END : MA_SUCCESS;
}

ma_result decoderSeek(ma_data_source* pDataSource, ma_uint64 frameIndex)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pDataSource;

	// This is what makes a looping track loop: at the end of the stream miniaudio seeks back to
	// frame 0 rather than reopening the file.
	if(stb_vorbis_seek(decoder->vorbis, (unsigned int)frameIndex) == 0)
		return MA_ERROR;

	return MA_SUCCESS;
}

ma_result decoderGetDataFormat(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels,
                               ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pDataSource;

	if(pFormat)
		*pFormat = ma_format_f32;   // stb_vorbis hands us floats; let miniaudio convert if it must
	if(pChannels)
		*pChannels = decoder->channels;
	if(pSampleRate)
		*pSampleRate = decoder->sampleRate;
	if(pChannelMap)
		ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, decoder->channels);

	return MA_SUCCESS;
}

ma_result decoderGetCursor(ma_data_source* pDataSource, ma_uint64* pCursor)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pDataSource;

	int offset = stb_vorbis_get_sample_offset(decoder->vorbis);
	*pCursor = (offset < 0) ? 0 : (ma_uint64)offset;

	return MA_SUCCESS;
}

ma_result decoderGetLength(ma_data_source* pDataSource, ma_uint64* pLength)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pDataSource;

	*pLength = (ma_uint64)stb_vorbis_stream_length_in_samples(decoder->vorbis);

	return MA_SUCCESS;
}

ma_data_source_vtable dataSourceVTable =
{
	decoderRead,
	decoderSeek,
	decoderGetDataFormat,
	decoderGetCursor,
	decoderGetLength,
	0,  /* onSetLooping */
	0   /* flags */
};

/// Pulls the whole compressed file off miniaudio's read callback — which is the engine's VFS. The
/// size is not asked for up front: an archive entry's idea of "tell" is its own, and reading until
/// the stream says it is done needs no such agreement.
ma_result slurp(ma_read_proc onRead, void* pUserData, const ma_allocation_callbacks* pAllocationCallbacks,
                unsigned char** ppData, size_t* pSize)
{
	const size_t CHUNK = 64*1024;

	unsigned char* data = 0;
	size_t size = 0;
	size_t capacity = 0;

	for(;;){
		if(size + CHUNK > capacity){
			size_t newCapacity = (capacity == 0) ? CHUNK : capacity*2;

			unsigned char* newData = (unsigned char*)ma_realloc(data, newCapacity, pAllocationCallbacks);
			if(!newData){
				ma_free(data, pAllocationCallbacks);
				return MA_OUT_OF_MEMORY;
			}

			data = newData;
			capacity = newCapacity;
		}

		size_t bytesRead = 0;
		ma_result result = onRead(pUserData, data + size, capacity - size, &bytesRead);

		size += bytesRead;

		if(result == MA_AT_END || bytesRead == 0)
			break;

		if(result != MA_SUCCESS){
			ma_free(data, pAllocationCallbacks);
			return result;
		}
	}

	if(size == 0){
		ma_free(data, pAllocationCallbacks);
		return MA_INVALID_FILE;
	}

	*ppData = data;
	*pSize = size;

	return MA_SUCCESS;
}

ma_result backendInit(void* /*pUserData*/, ma_read_proc onRead, ma_seek_proc /*onSeek*/,
                      ma_tell_proc /*onTell*/, void* pReadSeekTellUserData,
                      const ma_decoding_backend_config* /*pConfig*/,
                      const ma_allocation_callbacks* pAllocationCallbacks, ma_data_source** ppBackend)
{
	VorbisDecoder* decoder = (VorbisDecoder*)ma_malloc(sizeof(VorbisDecoder), pAllocationCallbacks);
	if(!decoder)
		return MA_OUT_OF_MEMORY;

	memset(decoder, 0, sizeof(*decoder));
	decoder->allocationCallbacks = *pAllocationCallbacks;

	size_t fileSize = 0;
	ma_result result = slurp(onRead, pReadSeekTellUserData, pAllocationCallbacks, &decoder->fileData, &fileSize);
	if(result != MA_SUCCESS){
		ma_free(decoder, pAllocationCallbacks);
		return result;
	}

	int error = 0;
	decoder->vorbis = stb_vorbis_open_memory(decoder->fileData, (int)fileSize, &error, 0);
	if(!decoder->vorbis){
		ma_free(decoder->fileData, pAllocationCallbacks);
		ma_free(decoder, pAllocationCallbacks);
		return MA_INVALID_FILE;   // not Vorbis, or corrupt: let miniaudio try its other backends
	}

	stb_vorbis_info info = stb_vorbis_get_info(decoder->vorbis);
	decoder->channels = (ma_uint32)info.channels;
	decoder->sampleRate = (ma_uint32)info.sample_rate;

	ma_data_source_config dataSourceConfig = ma_data_source_config_init();
	dataSourceConfig.vtable = &dataSourceVTable;

	result = ma_data_source_init(&dataSourceConfig, &decoder->ds);
	if(result != MA_SUCCESS){
		stb_vorbis_close(decoder->vorbis);
		ma_free(decoder->fileData, pAllocationCallbacks);
		ma_free(decoder, pAllocationCallbacks);
		return result;
	}

	*ppBackend = decoder;
	return MA_SUCCESS;
}

void backendUninit(void* /*pUserData*/, ma_data_source* pBackend, const ma_allocation_callbacks* pAllocationCallbacks)
{
	VorbisDecoder* decoder = (VorbisDecoder*)pBackend;
	if(!decoder)
		return;

	if(decoder->vorbis)
		stb_vorbis_close(decoder->vorbis);

	ma_data_source_uninit(&decoder->ds);
	ma_free(decoder->fileData, pAllocationCallbacks);
	ma_free(decoder, pAllocationCallbacks);
}

// No onInitFile / onInitFileW / onInitMemory: leaving them null keeps every read on the read
// callback above, which is the VFS — and the VFS is the only thing that can see inside a .pak.
ma_decoding_backend_vtable vorbisVTable =
{
	backendInit,
	0,  /* onInitFile */
	0,  /* onInitFileW */
	0,  /* onInitMemory */
	backendUninit
};

}

ma_decoding_backend_vtable* vorbisDecodingBackend()
{
	return &vorbisVTable;
}
