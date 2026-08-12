# Design

NailComb uses the DHN9 simple monochrome 8-bit visual system. The editor is a custom JUCE editor at 960 x 544 by default, with a 720 x 432 minimum size, 4 px base spacing with 8 px major spacing, no external images, and no external fonts. The only UI colors are the four-level palette `#050505`, `#2A2A2A`, `#8A8A86`, `#F2F2F0`. `GenericAudioProcessorEditor` is not used.

All eleven parameters are visible as host-attached controls with stable component IDs, names, and tooltips. The paint layer is intentionally minimal: product name at `y=16`, compact function label at `y=48`, and one 1 px divider at `y=72`; controls start at absolute `y=80`. Do not add a full-canvas grid, tagline, package ID, decorative motif, fake visualizer, fake meter, panel frame, outer border, or parameter-driven atmospheric drawing. DSP behavior, parameter IDs, bundle identity, accessibility, and host automation identity are not part of UI simplification and stay unchanged.

The audio callback boundary is deliberately narrow. `processBlock` reads APVTS atomics, sets sanitized DSP targets, clears unmatched output channels, and processes fixed channel/sample loops. `NailCombDSP::processSample` only performs bounded arithmetic, fixed four-voice iteration, preallocated delay-line access, finite guards, and one-pole smoothing. Delay-line allocation and resizing happen only in `prepare`.
