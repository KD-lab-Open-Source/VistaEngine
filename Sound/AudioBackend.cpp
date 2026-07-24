#include "StdAfx.h"
#include "AudioBackend.h"
#include "VorbisDecoder.h"

#include <mutex>

namespace {

// ---------------------------------------------------------------------------
// The VFS
// ---------------------------------------------------------------------------
// Every other asset in the game is read through XZipStream: it resolves a name against
// the mounted .pak archives first and the filesystem second, so the very path the scripts
// already use ("RESOURCE\SOUNDS\foo.wav") is the path that works. Handing miniaudio a VFS
// onto it is what puts the sounds in reach at all — all 252 of them live inside sound.pak
// and there is no loose copy on disk — and it retires the two bespoke bridges DirectSound
// needed for the same job: an mmio IO-proc for the .wav (Sound/WaveFile.cpp) and a
// separate ogg callback set (Game/SoundApp.cpp).
//
// miniaudio reads on its resource-manager job thread, concurrently with the game thread's
// own asset loads, and the archive manager behind XZipStream is shared mutable state — so
// serialize every VFS call under one lock. A coarse lock is affordable here: each pak
// entry is Stored rather than deflated, so a read is a memcpy off a file handle the stream
// owns, and a streaming sound asks for a few KB at a time.
std::mutex vfsMutex;

struct ZipVFS
{
	ma_vfs_callbacks callbacks;   // must be first: miniaudio casts ma_vfs* to this
};

ma_result vfsOpen(ma_vfs* /*pVFS*/, const char* path, ma_uint32 openMode, ma_vfs_file* pFile)
{
	if(openMode & MA_OPEN_MODE_WRITE)
		return MA_INVALID_OPERATION;

	std::lock_guard<std::mutex> lock(vfsMutex);

	// handleErrors = false: a missing sound has to come back as a result code, not as the
	// archive layer's own error box.
	XZipStream* stream = new XZipStream(false);
	if(!stream->open(path, XZS_IN)){
		delete stream;
		return MA_DOES_NOT_EXIST;
	}

	*pFile = stream;
	return MA_SUCCESS;
}

ma_result vfsOpenW(ma_vfs*, const wchar_t*, ma_uint32, ma_vfs_file*)
{
	return MA_NOT_IMPLEMENTED;   // the game names every asset in narrow chars
}

ma_result vfsClose(ma_vfs*, ma_vfs_file file)
{
	std::lock_guard<std::mutex> lock(vfsMutex);
	delete static_cast<XZipStream*>(file);
	return MA_SUCCESS;
}

ma_result vfsRead(ma_vfs*, ma_vfs_file file, void* dst, size_t sizeInBytes, size_t* pBytesRead)
{
	std::lock_guard<std::mutex> lock(vfsMutex);

	unsigned long read = static_cast<XZipStream*>(file)->read(dst, (unsigned long)sizeInBytes);
	*pBytesRead = read;

	// A short read is a success; only a read that returned nothing is the end of the file.
	if(read == 0 && sizeInBytes > 0)
		return MA_AT_END;

	return MA_SUCCESS;
}

ma_result vfsWrite(ma_vfs*, ma_vfs_file, const void*, size_t, size_t*)
{
	return MA_NOT_IMPLEMENTED;   // read-only: the game never writes audio
}

ma_result vfsSeek(ma_vfs*, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
{
	int dir;
	switch(origin){
	case ma_seek_origin_start:   dir = XZS_BEG; break;
	case ma_seek_origin_current: dir = XZS_CUR; break;
	case ma_seek_origin_end:     dir = XZS_END; break;
	default: return MA_INVALID_ARGS;
	}

	std::lock_guard<std::mutex> lock(vfsMutex);
	static_cast<XZipStream*>(file)->seek((long)offset, dir);
	return MA_SUCCESS;
}

ma_result vfsTell(ma_vfs*, ma_vfs_file file, ma_int64* pCursor)
{
	std::lock_guard<std::mutex> lock(vfsMutex);
	*pCursor = static_cast<XZipStream*>(file)->tell();
	return MA_SUCCESS;
}

ma_result vfsInfo(ma_vfs*, ma_vfs_file file, ma_file_info* pInfo)
{
	std::lock_guard<std::mutex> lock(vfsMutex);
	pInfo->sizeInBytes = static_cast<XZipStream*>(file)->size();
	return MA_SUCCESS;
}

// ---------------------------------------------------------------------------
// The device
// ---------------------------------------------------------------------------
ZipVFS zipVFS;
ma_resource_manager resourceManager;
ma_engine maEngine;
ma_sound_group groups[audio::GROUP_COUNT];
float groupVolumes[audio::GROUP_COUNT] = { 1.0f, 1.0f, 1.0f };

bool inited = false;
bool isMuted = false;

}

namespace audio {

bool init()
{
	if(inited)
		return true;

	zipVFS.callbacks.onOpen  = vfsOpen;
	zipVFS.callbacks.onOpenW = vfsOpenW;
	zipVFS.callbacks.onClose = vfsClose;
	zipVFS.callbacks.onRead  = vfsRead;
	zipVFS.callbacks.onWrite = vfsWrite;
	zipVFS.callbacks.onSeek  = vfsSeek;
	zipVFS.callbacks.onTell  = vfsTell;
	zipVFS.callbacks.onInfo  = vfsInfo;

	// The engine would spin up a resource manager of its own, but it would be one reading
	// the filesystem directly — and the sounds are not on the filesystem. Build it here so
	// it reads through the VFS above.
	ma_resource_manager_config resourceConfig = ma_resource_manager_config_init();
	resourceConfig.pVFS = &zipVFS;

	// Vorbis is not one of the formats miniaudio decodes for itself, and all of the music and all
	// of the speech is .ogg (Sound/VorbisDecoder.cpp). The effects are .wav, which it does handle.
	static ma_decoding_backend_vtable* customBackends[] = { vorbisDecodingBackend() };
	resourceConfig.ppCustomDecodingBackendVTables = customBackends;
	resourceConfig.customDecodingBackendCount = 1;

	if(ma_resource_manager_init(&resourceConfig, &resourceManager) != MA_SUCCESS){
		fprintf(stderr, "audio: ma_resource_manager_init failed\n");
		return false;
	}

	ma_engine_config engineConfig = ma_engine_config_init();
	engineConfig.pResourceManager = &resourceManager;

	if(ma_engine_init(&engineConfig, &maEngine) != MA_SUCCESS){
		fprintf(stderr, "audio: ma_engine_init failed\n");
		ma_resource_manager_uninit(&resourceManager);
		return false;
	}

	// DirectSound's world was left-handed. miniaudio's listener defaults to right-handed, and
	// the difference is not cosmetic: a left-handed listener negates the right-vector it builds
	// from (direction x worldUp), so getting this wrong swaps left and right in every pan. We
	// hand the listener the very vectors DirectSound was given (SND3DListener), so the
	// spatializer has to read them the same way. There is no setter for it; the field is public.
	for(ma_uint32 i = 0; i < ma_engine_get_listener_count(&maEngine); ++i)
		maEngine.listeners[i].config.handedness = ma_handedness_left;

	for(int i = 0; i < GROUP_COUNT; ++i){
		if(ma_sound_group_init(&maEngine, MA_SOUND_FLAG_NO_SPATIALIZATION, NULL, &groups[i]) != MA_SUCCESS){
			fprintf(stderr, "audio: ma_sound_group_init failed (group %i)\n", i);

			while(--i >= 0)
				ma_sound_group_uninit(&groups[i]);
			ma_engine_uninit(&maEngine);
			ma_resource_manager_uninit(&resourceManager);
			return false;
		}
	}

	inited = true;

	// A mute asked for before the device existed -- the window losing focus while the game
	// is still loading -- is a standing state, not something init gets to forget.
	ma_engine_set_volume(&maEngine, isMuted ? 0.0f : 1.0f);

	ma_device* device = ma_engine_get_device(&maEngine);
	fprintf(stderr, "audio: miniaudio %s on %s, %i ch @ %i Hz\n",
	        ma_version_string(),
	        ma_get_backend_name(device->pContext->backend),
	        (int)device->playback.channels,
	        (int)device->sampleRate);

	return true;
}

void release()
{
	if(!inited)
		return;

	for(int i = 0; i < GROUP_COUNT; ++i)
		ma_sound_group_uninit(&groups[i]);

	ma_engine_uninit(&maEngine);
	ma_resource_manager_uninit(&resourceManager);

	inited = false;
}

bool initialized()
{
	return inited;
}

ma_engine* engine()
{
	return inited ? &maEngine : 0;
}

ma_sound_group* group(Group group)
{
	xassert(group >= 0 && group < GROUP_COUNT);
	return inited ? &groups[group] : 0;
}

void setGroupVolume(Group group, float volume)
{
	xassert(group >= 0 && group < GROUP_COUNT);

	groupVolumes[group] = clamp(volume, 0.0f, 1.0f);

	if(inited)
		ma_sound_group_set_volume(&groups[group], groupVolumes[group]);
}

float groupVolume(Group group)
{
	xassert(group >= 0 && group < GROUP_COUNT);
	return groupVolumes[group];
}

void setMuted(bool muted)
{
	isMuted = muted;

	if(inited)
		ma_engine_set_volume(&maEngine, muted ? 0.0f : 1.0f);
}

bool muted()
{
	return isMuted;
}

}
