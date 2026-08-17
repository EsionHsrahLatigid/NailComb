# NailComb

NailComb is a tuned four-voice comb resonator with fractional delay interpolation, polarity-selectable feedback, damping, stereo detune, and bounded cross-coupling.

## Identity

- Product: `NailComb`
- Repository slug: `nailcomb`
- Bundle ID: `jp.ehl.nailcomb`
- Manufacturer: `EsionHsrahLatigid`
- Manufacturer code: `EHL_`
- Plugin code: `NlCb`
- Formats: VST3 and Standalone on supported platforms, plus AU on Apple

## Parameters

- `frequency` / Frequency: tuned comb fundamental in hertz.
- `fine` / Fine: cent offset around the tuned frequency.
- `feedback` / Feedback: resonant feedback amount, capped at `0.94`.
- `polarity` / Polarity: positive or negative feedback loop sign.
- `damping` / Damping: stable one-pole damping in the feedback path.
- `smear` / Smear: fixed inharmonic offsets across the four comb teeth.
- `stereoDetune` / Stereo Detune: opposite cent offsets for left and right.
- `crossCouple` / Cross-Couple: bounded stereo feedback coupling.
- `voiceSpread` / Voice Spread: cent spread across the four voices.
- `mix` / Mix: dry to comb-resonator blend.
- `trim` / Trim: post-mix output trim in decibels.

## Source Decisions

The comb core follows Julius O. Smith's PASP comb-filter and fractional-delay material: tuned delay loops create the resonant peak/notch train, while fractional interpolation keeps pitch control continuous. Hartmann's flanging/phasing context supports the polarity and moving-comb coloration choices. NailComb's implementation fixes the product constraints directly in code: four voices, pitch/frequency control, feedback polarity, damping, stereo detune, cross-coupling, and strict sub-unity feedback.

Implementation mapping:

- Four feedback comb voices: `nailcomb::dsp::NailCombDSP::voiceCount`.
- Fractional delay timing: linear interpolation in `readFractional`.
- Pitch/frequency control: `frequency`, `fine`, `voiceSpread`, `smear`, and `stereoDetune`.
- Polarity response: `polarity` multiplies the feedback loop.
- Damping: one-pole feedback-path damping controlled by `damping`.
- Stereo character: per-channel detune plus bounded previous-sample cross-coupling.
- Stability: `feedback` is clamped to `0.94`; `crossCouple` is clamped to `0.35`; delay lengths are bounded away from zero; all input and output samples are sanitized and clamped.

## Build

```sh
cmake --preset engine-debug
cmake --build --preset engine-debug
ctest --preset engine-debug --output-on-failure

cmake --preset plugin-release -DEHL_JUCE_SOURCE_DIR=/path/to/JUCE
cmake --build --preset plugin-release --target ehl_stage_products
ctest --preset plugin-release --output-on-failure
```

The project pins JUCE to `91ad83ae34a81e0833b1a2b0866f54846370ae53` when network FetchContent is used. Set `EHL_JUCE_SOURCE_DIR` for offline builds.

On local macOS builds, VST3 and AU products are copied after build to the current user's standard Audio Plug-Ins folders. CI and non-macOS builds leave this off by default. Override with `-DEHL_COPY_PLUGIN_AFTER_BUILD=ON|OFF`; Standalone products are only staged under the artifact tree.

Stable artifacts:

```text
artifacts/plugin-release/macos-arm64/standalone/nailcomb_standalone_plugin.app
artifacts/plugin-release/macos-arm64/vst3/nailcomb_vst3_plugin.vst3
artifacts/plugin-release/macos-arm64/au/nailcomb_au_plugin.component
artifacts/plugin-release/macos-arm64/ARTIFACTS.txt

artifacts/plugin-release/windows-x64/standalone/nailcomb_standalone_plugin.exe
artifacts/plugin-release/windows-x64/vst3/nailcomb_vst3_plugin.vst3
artifacts/plugin-release/windows-x64/ARTIFACTS.txt
```

## Tests

- `nailcomb_dsp_tests`: deterministic comb timing, polarity, damping, stereo detune, feedback bounds, finite rendering, reset, and non-finite sanitization.
- `nailcomb_plugin_tests`: identity, APVTS state, parameter presence, matched mono/stereo bus policy, finite processing, and tail/latency contracts.
- `nailcomb_editor_tests`: custom editor, all parameter controls, tooltips, accessibility names, the shared EHL parameter display, and rotary control layout.
- `nailcomb_artifact_contract`: staged VST3/Standalone/AU artifact paths and macOS codesign checks.
