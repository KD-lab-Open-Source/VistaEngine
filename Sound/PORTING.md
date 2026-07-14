# Audio porting register — DirectSound out, miniaudio in

DirectSound is gone. **miniaudio's `ma_engine` is the audio device on every platform, Windows
included** — the same play as the renderer's D3D9 → SDL GPU move: keep the engine's own interface
(`SoundSystem` / `Sound` / `Channel` / `SND3DListener` / `OggPlayer`, ~100 call sites), swap the
guts underneath it.

SDL's audio subsystem stays **off** (`SDL_INIT_AUDIO` is not requested). SDL3 *does* mix, so "no
mixer" was never the real gap — the gap is everything above the mixer: Vorbis decoding, 3D
spatialization, a voice pool, fades, and a streaming resource manager. `ma_engine` covers that set
almost one-for-one, and its `ma_attenuation_model_inverse` *is* DirectSound's inverse-distance law.

Nothing is lost. The DirectSound sources stay in the tree, unbuilt, as the reference — exactly as
the D3D9 sources do for the renderer. **Read the original before porting a behaviour:**

| Reference (not built) | Was |
|---|---|
| `Sound/SoundSystem.cpp` | the device, the sample pool, the voices |
| `Sound/C3D.cpp`, `Sound/HardwareBuffer.cpp` | the DirectSound3D listener and 3D buffers |
| `Sound/SoftwareBuffer.cpp` | a second, orphaned 3D mixer — see "Never shipped" below |
| `Sound/WaveFile.cpp` | the `mmio` IO-proc that read `.wav` out of a `.pak` |
| `Sound/init.cpp` | the `SND*` entry points |
| `XLibs.Net/OGG/PlayOgg/PlayOgg.cpp` | the Vorbis streaming player and its thread |

---

## What came across

| # | Feature | Now lives in |
|---|---------|--------------|
| 1 | **The device** | `Sound/AudioBackend.{h,cpp}` — `ma_engine` over a `ma_resource_manager` whose `ma_vfs` is backed by **`XZipStream`**, plus three `ma_sound_group` volume buses (SFX / MUSIC / VOICE). One coarse mutex guards every VFS call: miniaudio reads on its own job thread, and the archive manager is shared mutable state. |
| 2 | **Sound effects** | `Sound/SoundSystemMiniaudio.cpp`. A `Sound` is one sample decoded once (`MA_SOUND_FLAG_DECODE`); a `Channel` is a voice cloned off it with `ma_sound_init_copy`. All 252 effects load. |
| 3 | **3D sound** | Same file. `SND3DListener` drives miniaudio's listener; 3D voices are spatialized, UI ones are not (`MA_SOUND_FLAG_NO_SPATIALIZATION`). |
| 4 | **Music, speech and cutscene audio** | `Sound/OggPlayerMiniaudio.cpp` — one streaming `ma_sound` per player (`MA_SOUND_FLAG_STREAM`). The original's streaming thread and its hand-ticked fade both disappear into miniaudio. |
| 5 | **Vorbis** | `Sound/VorbisDecoder.{h,cpp}` — miniaudio decodes wav/flac/mp3 but **not Vorbis**, so this registers a decoding backend built on **stb_vorbis**, which miniaudio already ships at `extras/stb_vorbis.c`. No new dependency. |

The VFS (#1) is what retires *both* bespoke IO bridges the original carried: the `mmio` IO-proc for
`.wav` (`Sound/WaveFile.cpp`) and the ogg callback set that used to sit in `Game/SoundApp.cpp`. Every
sound in the game — including the ones that exist only inside a `.pak` — is now read one way.

Registering the Vorbis backend **on the resource manager** (rather than on individual decoders) is
what makes speech stream straight out of `voices_*.pak`: it inherits the VFS for free.

### The volume curve is ported exactly, not approximated

DirectSound took millibels, and the game fed it through `CalcVolume()` (0..255, log curve). Folding
that together with DirectSound's own `gain = 10^(mB/2000)` collapses to:

```
gain = t^5 / 1e5,  where t = ln(vol + 1) * 1.623031921 + 1,  vol = int(v01 * 255)
```

The entire sound library was mixed against this curve and it is audibly louder than a linear fader.
A sound configured at 0.7 must come out at gain 0.741466. `PlayOgg`'s `ToDirectVolume()` and the
game's `CalcVolume()` are the same function written twice, so the **music uses the same curve**.

---

## Never shipped — do not "port" these

Three features are serialized, editable, and **wired to nothing**. Implementing them would not be
porting; it would be inventing behaviour the game never had.

| Feature | Why it never ran |
|---|---|
| **`zmultiple`** (`SND3DListener::SetZMultiple`, `fSoundZMultiple`) | Read *only* by `SoftwareBuffer.cpp`, which belongs to a second 3D implementation (`SND3DSound` / `VirtualSound3D` / `SoftSound3D` / `HardSound3D`) that **nothing ever constructs**. It is commented out in the hardware path that actually ran. `Game/SoundApp.cpp` still calls `SetZMultiple` — the call is kept, and does nothing, exactly as before. |
| **Doppler** | The doppler factor is never sent to DirectSound, `SoundQuant` hardcodes the listener velocity to `Vect3f(0,0,0)`, and no emitter ever sets one. |
| **`frequencyRnd`** (pitch randomisation) | Serialized into the sound attributes, read by nothing. |

## Dead in the shipped source — not ours to restore

**The reels (Bink cutscenes) do not play, and never did in this source tree.** KD-lab stripped the
Bink player before releasing the source:

- `UserInterface/ReelManager.cpp` — `showModal()` opens with an unconditional `return;`
- `Game/PlayBink.cpp` — `PlayBinkR::Init()` opens with `return false;`, and every other method is
  empty. The file is not in the build (it includes `d3d9.h`).

So this is a **video** gap, not an audio one. Worth knowing: a reel's soundtrack is a plain `.ogg`
played through `gb_Music` — the *audio* half of the cutscene path is already ported and working
(#4). If the video is ever restored, the sound will come with it; there is nothing to write here.

---

## What miniaudio will not do, which we do ourselves

All four are implemented in `Sound/SoundSystemMiniaudio.cpp`.

| # | Behaviour | Note |
|---|---|---|
| 6 | **The per-sound voice cap** | `maxCount_` / `FindFreeChannel` — miniaudio has no voice-stealing policy. |
| 7 | **Mute past max distance** | DirectSound had `DSBCAPS_MUTE3DATMAXDISTANCE`. miniaudio *clamps* attenuation at max distance rather than cutting off, so the clip-distance pass mutes the voice itself. |
| 8 | **Fog of war** | `stopInFogOfWar_`. The Sound module asks the game through the `SNDSetFogOfWarQuery` hook, rather than reaching into the universe as the original did — that keeps `Game/Universe.h` out of `Sound/`. |
| 9 | **The focus mute** | DirectSound self-silenced when the window lost focus; miniaudio does not. Driven from `applicationHasFocus()` in `SoundSystem::Update`. |

---

## Traps found in the original — read before you touch this

- **`SNDSetGameActive` does not mean "window focused".** It means "a mission is running", and
  `GameShell` clears it on returning to the menu. Muting the device on it silences the entire main
  menu (which is itself a running mission). It gates fog-of-war and nothing else. The focus mute is
  a separate thing — see #9.
- **`Sound`'s constructor set `volume_ = DSBVOLUME_MAX`**, which reads as "loudest" but *is the
  constant `0`*, and it fed a 0..1 scale. An unconfigured `Sound` was therefore silent.
- **`MusicManager::Play` probed every track with a raw `fopen()` on a backslash path**
  (`Resource\Music\…`), which finds nothing off Windows — music would have stayed silent even with a
  perfect player. It probes through `XZipStream` now, like everything else.
- **Hardware 3D needs no special library.** DirectSound's hardware path was removed in Windows
  Vista, so the original already ran software-mixed; it asked for `DS3DALG_DEFAULT` and left
  `DS3DALG_HRTF_FULL` commented out. miniaudio's spatializer is parity. True binaural HRTF *would*
  need a library — miniaudio ships a Steam Audio integration if that is ever wanted.
- **Handedness is a real trap.** DirectSound's world is **left-handed**; miniaudio's listener
  defaults to right-handed and *negates* the right-vector it derives from `direction × worldUp`. Get
  it wrong and left/right silently swap, with nothing to see. `AudioBackend.cpp` sets
  `listener.config.handedness = ma_handedness_left` (there is no setter; the field is public).
  Verify by reading the real gains, not by reasoning:
  `sound->engineNode.spatializer.pNewChannelGainsOut[0/1]` — an emitter to the camera's right must
  give right 1.0 / left 0.2.
- **`grep` lies about the CP1251 files.** It treats them as binary and silently skips them, so a
  symbol can look nonexistent when it is sitting right there — this is how `kdWarning` (a macro in
  `XLibs.Net/XUtil/Console.h`) came to look undeclared. **Always `grep -a`.** And never edit anything
  under `XLibs.Net/` with a text editor that rewrites the file: it will re-encode CP1251 to UTF-8 and
  destroy the Cyrillic. Patch those as bytes.

---

## Asset facts that drove the design

- **SFX**: 252 × mono 16-bit PCM `.wav`, and they exist **only inside `sound.pak`** — the
  `Resource/Sounds/` directories on disk are empty. This is why the VFS was mandatory rather than a
  nicety, and why it was the first thing built and verified. Mono is also what a spatializer needs.
- **Music**: 15 × stereo 44.1 kHz Vorbis `.ogg`, 58 MB, loose in `Resource/Music/` → must stream.
- **Voice**: Vorbis inside per-language `voices_{en,de,es,fr,it}.pak` → streamed *out of an archive*.
- Every `.pak` entry is **Stored**, not deflated, so `XZipStream` reads it through its own file
  descriptor at an offset — real random access per stream, which is exactly what a VFS wants.
- **Voice durations are shipped, not measured.** `Resource/LocData/<lang>/Voice/voiceDurations` is a
  table the briefings pace themselves by; `OggPlayer::getLength` only feeds the editor's
  regenerate-the-table path (`saveVoiceFileDurations`). If that table fails to load, every line of
  speech silently falls back to 1 second.

## One shortcut, deliberately taken

stb_vorbis **cannot read through callbacks** — it wants a filename, a `FILE*`, or a block of memory,
and inside a `.pak` we have none of those. So `VorbisDecoder.cpp` pulls the *compressed* file into
memory once and decodes from there. That is less extravagant than it sounds: a music track is ~4 MB
compressed against roughly 80 MB of PCM, and it is the PCM that matters — that is still decoded a
page at a time as the track plays. Only the one playing track and the one line of speech are ever
resident.
