# Design

NailComb uses the shared DHN9 monochrome 8-bit visual system with its own nail-and-teeth spectral motif. The editor is a custom JUCE editor at 960 x 544 by default, with a 720 x 432 minimum size, 8 px grid, grayscale palette, procedural drawing, no external images, and no external fonts. `GenericAudioProcessorEditor` is not used.

All eleven parameters are visible as host-attached controls with stable component IDs, names, and tooltips. The visual motif mirrors the DSP: horizontal spectral bars show the current comb state and the lower teeth row suggests the sharp resonant delay bank.

The audio callback boundary is deliberately narrow. `processBlock` reads APVTS atomics, sets sanitized DSP targets, clears unmatched output channels, and processes fixed channel/sample loops. `NailCombDSP::processSample` only performs bounded arithmetic, fixed four-voice iteration, preallocated delay-line access, finite guards, and one-pole smoothing. Delay-line allocation and resizing happen only in `prepare`.
