# PhatForge

An original, from-scratch **VST3 / AU / Standalone** "phattening" multi-effect
plugin built with [JUCE](https://juce.com), inspired by the *feature set* of
classic colouring multi-effects (distortion modules + low-end enhancer EQ +
compressor + resonant filters + LFOs + envelope follower + X/Y pad). None of
this code is copied, decompiled, or reverse-engineered from any third-party
plugin binary — it's a clean-room implementation you can build, modify, and
relicense yourself.

## Why this exists

You can't legally take a closed-source, commercial plugin binary (e.g. the
discontinued Camel Audio CamelPhat, whose IP now belongs to Apple/Logic Pro's
"Phat FX") off a piracy site and "convert" it into VST3 — that's redistributing
someone else's copyrighted software. What you *can* do is build your own plugin
that captures the same idea. That's what this project is.

## Feature set

- **3 resonant filters** in series, each independently switchable between
  **8 types**: Low-pass, High-pass, Band-pass, Notch, Peak/Bell, Low Shelf,
  High Shelf, and a resonant feedback **Comb** filter — each with its own
  frequency, resonance (doubles as gain for the bell/shelf types) and on/off
  switch.
- **8 distortion modules** — Tube (tanh saturation), Crush (bit-depth + sample
  -rate reduction), Exciter (upper-harmonic generator), Mech (hard clip),
  Diode (asymmetric diode-style clamp), Fold (triangle wavefolder), Rectify
  (fuzz-rectifier octave-up blend), and Shaper (Chebyshev-style polynomial
  waveshaper morphing from warm to buzzy via Tone) — with Drive, Tone and Mix.
- **Magic EQ** — a low-end enhancer that blends a saturated low band back in
  under a low-shelf boost, for a "sub" feel that still reads on small speakers.
- **Compressor** — threshold, ratio, attack, release, makeup gain.
- **2 LFOs** (sine/triangle/square/saw) and an **envelope follower**, each
  routable to any of: filter 1/2/3 frequency, distortion drive, Magic EQ
  amount, or compressor threshold.
- **X/Y pad** for quick, expressive real-time control of drive + filter 1 freq.
- **Custom-painted GUI** — a bespoke `PhatLookAndFeel` (no stock JUCE widget
  graphics): glowing arc-style rotary knobs, LED-style toggles, pill-shaped
  combo boxes, a gradient header with a hand-drawn logo mark, and a live
  peak-with-hold stereo output meter.
- **Randomize** button.
- **Preset management** — save/load/delete named presets to disk (a real
  preset browser, not just DAW session recall), with prev/next stepping and
  a handful of factory presets seeded on first run.
- Global input/output gain and a master dry/wet mix.

## Presets

- `PresetManager.h/.cpp` saves/loads presets as plain XML files (the file
  extension is `.phatpreset`) in a `PhatForge/Presets` folder inside your
  OS's standard application-data directory (e.g. `~/Library/Application
  Support/PhatForge/Presets` on macOS, `%APPDATA%\PhatForge\Presets` on
  Windows, `~/.config/PhatForge/Presets` on Linux).
- On first run it seeds a few factory presets — **Init**, **Warm Bass
  Glue**, **Crunch Drums**, **Robo Comb Riser**, **Sub Enhancer**, **Diode
  Grind Guitar** — so there's something to explore immediately. It never
  overwrites a preset that already exists on disk, including if you edit
  and re-save over a factory one.
- The preset bar (`PresetBar.h`) sits just under the header: `<` / `>` step
  through presets alphabetically, the dropdown jumps straight to one, `SAVE`
  prompts for a name (overwrites if it already exists), and `DEL` removes
  the currently-loaded preset after a confirmation.
- The current preset name is also stored in the plugin's regular DAW state
  (`getStateInformation`/`setStateInformation`), so reloading a saved song
  shows the right preset name in the bar even though the actual parameter
  values come from the session itself, not the disk file.

## GUI architecture

- `PhatLookAndFeel.h` — the entire visual identity in one place: colour
  palette, custom-painted rotary knobs (glow arc + pointer), toggle LEDs,
  combo boxes and buttons. Swap the colours in `PhatLookAndFeel::colours` to
  re-skin the whole plugin in one go.
- `PanelComponent.h` — a hand-painted card/group panel (rounded, drop shadow,
  accent stripe + title) used to group each effect section.
- `LevelMeter.h` — a small custom stereo peak/hold meter that polls the
  processor's lock-free level atomics at 30Hz.
- `XYPad.*` — the X/Y macro pad, custom-painted with a glowing draggable dot.

## Project layout

```
PhatForge/
├── CMakeLists.txt              JUCE + FetchContent build script
├── .github/workflows/build.yml CI: builds VST3/AU/Standalone on Win/Mac/Linux
├── Source/
│   ├── PluginProcessor.h/.cpp  Audio engine, parameter handling, state
│   ├── PluginEditor.h/.cpp     GUI layout / wiring
│   ├── PluginParameters.h      Central parameter ID list + APVTS layout
│   ├── PhatLookAndFeel.h       Custom-painted knobs/toggles/combos/buttons
│   ├── PanelComponent.h        Custom-painted section panel
│   ├── LevelMeter.h            Custom-painted stereo peak/hold meter
│   ├── PresetManager.h/.cpp    Save/load/delete presets + factory presets
│   ├── PresetBar.h             Preset browser bar GUI
│   ├── XYPad.h/.cpp            X/Y pad GUI control
│   └── DSP/
│       ├── Distortion.h        8 distortion algorithms
│       ├── MagicEQ.h           Low-end enhancer
│       ├── FilterSection.h     8-type switchable resonant filter
│       ├── ModLFO.h            Free-running LFO
│       └── EnvelopeFollower.h  One-pole envelope follower
```

## Building it

This sandbox environment has no general internet access (its shell can't
reach github.com/apt/pip mirrors), so the actual compilation couldn't be run
*here*. The two easiest ways to get a real, working `.vst3` file:

### Option A — GitHub Actions (recommended, zero local setup)

1. Push this folder to a new GitHub repo.
2. GitHub Actions will automatically run `.github/workflows/build.yml`, which
   builds Release VST3/AU/Standalone binaries for Linux, Windows and macOS.
3. Download the built plugin from the workflow run's **Artifacts** section.

### Option B — Build locally

Requires a C++17 compiler and CMake ≥ 3.22, plus an internet connection (to
fetch JUCE the first time you configure).

```bash
git clone <your-repo-url> PhatForge
cd PhatForge
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The built plugin will appear under `build/PhatForge_artefacts/Release/VST3/`
(JUCE's `COPY_PLUGIN_AFTER_BUILD` option is also on, so it will additionally
be copied straight into your system's VST3 folder).

**Linux build dependencies** (not needed on Windows/macOS):
```bash
sudo apt-get install libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
  libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev
```

If you already have a local JUCE checkout and don't want CMake to download
one, configure with:
```bash
cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
```

## Extending it

- Add more distortion algorithms in `Source/DSP/Distortion.h`.
- Add more modulation targets by extending the `ModTarget` enum and
  `modTargetChoices()` in `PluginParameters.h`, then handle the new target
  in `PluginProcessor::processBlock`.
- Swap the basic knob-and-label GUI in `PluginEditor.cpp` for a custom-painted
  look — the DSP/parameter layer is fully decoupled from the GUI.

## License

The original code in this repository is provided under the MIT license (see
`LICENSE`). JUCE itself is fetched separately at build time and is subject to
its own licensing terms (GPLv3 or the JUCE commercial license) — see
https://juce.com/juce-8-licence for details.
