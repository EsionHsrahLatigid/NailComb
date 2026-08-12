# Design

NailComb uses the shared EHL JUCE design module. The editor is a custom JUCE editor at 640 x 360 by default, with a 512 x 320 minimum size, 4 px base spacing with 8 px major spacing, no external images, and no external fonts. The only UI colors are the four-level palette `#050505`, `#2A2A2A`, `#8A8A86`, `#F2F2F0`. `GenericAudioProcessorEditor` is not used.

All eleven parameters are visible as host-attached controls with stable component IDs, names, and tooltips. The paint layer is intentionally minimal: product name begins at `y=8`, compact function label at `y=32`, and one 1 px divider at `y=56`; controls start at absolute `y=64`. Do not add a full-canvas grid, tagline, package ID, decorative motif, fake visualizer, fake meter, panel frame, outer border, or parameter-driven atmospheric drawing. DSP behavior, parameter IDs, bundle identity, accessibility, and host automation identity are not part of UI simplification and stay unchanged.

The audio callback boundary is deliberately narrow. `processBlock` reads APVTS atomics, sets sanitized DSP targets, clears unmatched output channels, and processes fixed channel/sample loops. `NailCombDSP::processSample` only performs bounded arithmetic, fixed four-voice iteration, preallocated delay-line access, finite guards, and one-pole smoothing. Delay-line allocation and resizing happen only in `prepare`.
