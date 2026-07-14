#ifndef __AUDIO_BACKEND_H_INCLUDED__
#define __AUDIO_BACKEND_H_INCLUDED__

#include "miniaudio.h"

/// The audio device, on every platform including Windows: one miniaudio engine, one
/// VFS onto the game's own file/archive streams, and the mix buses the volume options
/// map onto. DirectSound is retired; nothing above this file names it.
///
/// SDL owns the window, the input and the GPU, but deliberately not the audio device.
/// SDL3 mixes the streams bound to a device and offers nothing above that, while the
/// engine's sound API (SoundSystem / Sound / Channel / SND3DListener) is a skin over
/// DirectSound3D: per-emitter distances, rolloff, doppler, fades, a voice pool. That is
/// the set ma_engine covers.
namespace audio {

/// The three mix buses the volume options map onto. A group is a volume bus and nothing
/// more — the sounds inside it do their own spatialization.
enum Group
{
	GROUP_SFX,
	GROUP_MUSIC,
	GROUP_VOICE,
	GROUP_COUNT
};

bool init();
void release();
bool initialized();

ma_engine* engine();
ma_sound_group* group(Group group);

/// volume = 0..1
void setGroupVolume(Group group, float volume);
float groupVolume(Group group);

/// Silences everything without disturbing the per-group volumes. Both the sound option
/// going off and the window losing focus come back through here.
void setMuted(bool muted);
bool muted();

}

#endif
