# NailComb Research And Decision Map

NailComb implements the DHN9 comb-filter product from the G001 identity plan.

## Sources

- Julius O. Smith, PASP comb filters: supports feedforward/feedback comb behavior and the resonant spectral teeth created by delay feedback.
- Julius O. Smith, PASP fractional delay and delay interpolation: supports fractional delay-line tuning instead of integer-only pitch steps.
- William M. Hartmann AES flanging/phasing context: supports the perceptual role of polarity, notches, and comb-like coloration.
- DHN9 G001 evidence: fixes the product identity, four fractional feedback comb voices, polarity, damping, stereo detune, cross-coupling, and strict sub-unity feedback constraints.
- DHN9 G002 foundation evidence: fixes the CMake/JUCE scaffold, artifact contract, custom editor requirement, and local deterministic test surfaces.

## Product Interpretation

The effect is a tuned resonator and tearing notch bank, not a reverb, phaser, flanger, or static EQ. The sound comes from four fractional feedback loops whose delay lengths are derived from the chosen frequency plus fine, spread, smear, and stereo detune offsets.

## Safety Decisions

- Feedback is clamped to `0.94`, below unity.
- Cross-coupling is limited to `0.35` and mixed with self-feedback inside the same bounded loop.
- The minimum fractional delay is two samples, preventing zero-delay feedback.
- Delay memory is allocated during `prepare`, then reused in `processSample`.
- The audio path performs no locking, logging, file/network I/O, heap allocation, or unbounded history traversal.
- Non-finite input and state are sanitized; output is finitely clamped.
- Reset clears all delay memories and damping states.

## Test Mapping

- Tuned timing: 1000 Hz impulse repeat at 48 samples for 48 kHz.
- Polarity: the second repeat flips sign when feedback polarity is negative.
- Damping: high damping reduces feedback-tail energy against the bright setting.
- Stereo detune: left and right delay targets diverge by a measurable amount.
- Feedback bound: hostile feedback and cross-couple targets are capped.
- Stability: long mono render, silence, reset, NaN, and infinity cases remain finite.
