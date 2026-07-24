# Video: Bink and AVI, on ffmpeg

The companion to `Sound/PORTING.md` and `Render/PORTING.md`. This one records what the video
port had to reckon with, and what it deliberately did not do.

The game has video in two places, and they have nothing to do with each other:

| | what it is | who plays it |
|---|---|---|
| **`.bik`** | Bink 1 video (105 files) | the briefing panels (`UI_StreamVideo`) and the full-screen reels (`ReelManager::showModal`) |
| **`.avi`** | *not* a cutscene: 26 animated **textures** — snowflakes, water rings, coast bubbles, animated UI | `cAVIImage` in `Render/src/FileImage.cpp`, packed into an atlas by `cAviScaleFileImage` |

Both were Windows-only. Bink went through RAD's `binkw32.dll`, AVI through Video-for-Windows.

## What KD-lab stripped, and what that cost

**The Bink player was removed from the source release, not disabled.** What shipped was its
outline with the RAD calls cut out of it:

- `UI_StreamVideo.cpp` — a decode thread, two frame textures, a phase, a volume, all empty.
  Its `init()` returned **success** without loading anything, and the port before this one
  leaned on that: the briefing screens' trigger chains stall while a video control is
  un-inited, so a fake "loaded" video was what let *Select Mission* reveal its mission nodes
  and its START button at all.
- `ReelManager::showModal()` — an unconditional `return;`.
- `Game/PlayBink.cpp` — every method empty, `PlayBink::Create()` never defined anywhere.

So the intro has never played in this tree, nor has any mission cutscene. All three are live
again; `PlayBink` itself is deleted, being an abstraction over a library we no longer use.

## The decisions

**ffmpeg, built from source with almost all of it turned off.** Two demuxers and three
decoders — `bink`, `avi`, `binkvideo`(`bink`), `binkaudio_dct`, `rawvideo` — plus swscale, and
under 3 MB of static library comes out. The configure line and the reasoning are in
`cmake/FFmpeg.cmake`, which is also where the Windows story lives (MSVC compiles it; MSYS2 only
supplies the shell its configure script needs).

**No protocols.** Every file is opened through `XZipStream` with a custom `AVIOContext`, so a
video inside a `.pak` is as reachable as a loose one — and a protocol-less ffmpeg *cannot*
quietly bypass the VFS by opening a path, which is the failure mode that would hide the mistake.

**No decode thread.** The original had one because Bink's API had to be pumped to keep its own
audio fed. Ours does not: `VideoFile` decodes the whole soundtrack at `open()`, hands it to
miniaudio in one piece, and the audio device becomes the clock. `VideoPlayer::quant()` then
pulls video frames against that clock on the game thread. The soundtrack of the longest video in
the game (the 132-second intro) is 23 MB of PCM, which is the entire price of this.

A video with **no** soundtrack — three of them — runs off the wall clock instead.

**Losing that thread is also what moved focus handling.** The thread was where the original
handled the window going to the background, and it is worth reading:

```cpp
// BinkSimplePlayerImpl::threadProc, at ca9aa43
if(!pPlayer->getPause() && applicationHasFocus()){
    ...
    pPlayer->quant();          // decode, and feed Bink's audio
}
else
    pPlayer->setVolume(0.f);
```

Unfocused, it simply stopped pumping — so Bink's picture *and* its soundtrack stalled together,
and the brief came back exactly where it was left. That falls out of a design where the audio is
pumped; it does not fall out of ours, where the soundtrack is handed to miniaudio whole and plays
on miniaudio's thread while the frame loop that pulls the picture is frozen (see the focus section
in `Sound/PORTING.md` for why the frame loop freezes). Left alone, the picture stops and the voice
runs to the end without it.

So the brief is **paused** on focus loss, not muted: `UI_StreamVideo::setApplicationActive()`, from
`GameShell::onSetFocus()`. `VideoPlayer::pause()` stops the sound and the cursor stays put, which
is what keeps the two together — the audio *is* the clock, so pausing it pauses the video by
construction. Note the composition: focus and the UI's own pause are two independent conditions
that either can hold, and neither writes the other's flag, exactly as the `&&` above has it.

**Alpha comes from the pixel format.** 96 of the 105 `.bik` files carry an alpha plane, and the
UI blend mode depends on knowing that (`UI_ControlVideo::redraw`). The original read Bink's
`BINKALPHA` flag (`1<<20`); we ask the decoder, which answers by choosing `yuva420p` over
`yuv420p`. Same bit, from the other end.

## Traps worth knowing

- **A raw-DIB `.avi` decodes bottom-up.** ffmpeg hands the rows over with a *negative* stride and
  swscale turns them the right way up, which is the flip the VFW path used to do by hand
  (`cFileImage_GetFrame`'s `vInvert`, now `false` where it used to be `true`).
- **`PeekMessage` is a stub off Windows.** It returns FALSE, so a modal loop built on it — which
  is what a full-screen reel is — would pump nothing: no abort key (it arrives as a `WM_KEYDOWN`
  through `GameShell::checkReel`), no window close, and an application that looks hung for the
  length of the video. `showModal` calls `pumpApplicationEvents()` (`Game/Runtime.h`) instead.
  **`showPictureModal` and `showLogoModal` still have this bug** — they were left alone because
  neither is reachable (nothing calls them) and `showLogoModal` is D3D9 code besides.
- **`DisableVideo` defaulted to `true`** (`Game/IniFile.cpp`), which made sense only while there
  was no player. It is `false` now — but an `iniFile.cfg` already on disk overrides the default,
  so an existing install still needs the line changed by hand.
- **The video gate hides more than video.** `isVideoEnabled()` guards *two* trigger actions, and
  only one of them is a video: `ActionShowReel` (ported, above) and **`ActionShowLogoReel`**, the
  KD-lab logo splash, which is a D3D9 *rendering* feature and is not ported. Turning the reels on
  is what first fires it, and it went straight into a null `gb_RenderDevice3D`.
  `ReelManager::showLogoModal` is now guarded off; the register entry is **#23 in
  `Render/PORTING.md`**, where it belongs. Expect more of this: `DisableVideo = true` was load-
  bearing in ways that have nothing to do with decoding a `.bik`.

## Not done

- **No Bink 2 (`KB2`).** Nothing in the game is Bink 2 — every `.bik` here is `BIKi`, Bink 1 —
  and ffmpeg has no Bink 2 decoder anyway.
- **`binkaudio_rdft` is not built.** Bink's other audio codec; every soundtrack in this game is
  DCT. One word in `cmake/FFmpeg.cmake` if that ever stops being true.
- **`--disable-x86asm`.** ffmpeg's hand-written SIMD is off, so nobody has to install nasm. The
  pure-C decoder does 800x600@30 in a couple of ms a frame, which is the largest thing we ask of
  it. Turning it back on means adding nasm to all three CI jobs.
- **The AVI *writer* is gone.** `cFileImage::save()` on an `.avi` had no callers, and the ffmpeg
  we build has no encoders or muxers. `sVideoWrite` (`Render/src/WinVideo.cpp`, the D3D9-era
  screen-capture recorder) is a separate thing, still stubbed in `RenderStub.cpp`.
