# Design

NailComb uses the shared EHL JUCE design module. The editor is a custom JUCE editor at 640 x 360 by default, with a 512 x 320 minimum size, 4 px base spacing with 8 px major spacing, no external images, and no external fonts. The only UI colors are the four-level palette `#050505`, `#2A2A2A`, `#8A8A86`, `#F2F2F0`. `GenericAudioProcessorEditor` is not used.

All eleven parameters are visible as host-attached controls with stable component IDs, names, and tooltips. The paint layer provides only shared chrome: product name, compact function label, brand mark, and the 1 px divider. Below it, the V2 `ParameterDisplay` uses `DisplayKind::comb` and is driven only from message-thread reads of real APVTS-attached slider values: `Frequency`, `Feedback`, `Cross-Couple`, and `Voice Spread`, each normalized with `Slider::valueToProportionOfLength`. This display is truthful parameter state, not an audio meter, waveform scope, DSP tap, or audio-thread visualizer.

Controls use the shared rotary dial style with `TextBoxBelow` values and shared `controlCell()` geometry below the display. Do not add a full-canvas grid, tagline, package ID, decorative motif, fake meter, panel frame, outer border, or audio-reactive drawing. DSP behavior, parameter IDs, bundle identity, accessibility, and host automation identity are not part of UI simplification and stay unchanged.

The audio callback boundary is deliberately narrow. `processBlock` reads APVTS atomics, sets sanitized DSP targets, clears unmatched output channels, and processes fixed channel/sample loops. `NailCombDSP::processSample` only performs bounded arithmetic, fixed four-voice iteration, preallocated delay-line access, finite guards, and one-pole smoothing. Delay-line allocation and resizing happen only in `prepare`.
