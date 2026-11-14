# Bounded memory probe

Experimental MEM WRITE (`XXMEMWR`, playground ID `07001050`). One bounded hardware run passed all five phases; see [Stage E results](../../../zoom-midiverb-planning/doc/delay-abi-investigation/stage-e-results.md) for the exact artifact, recording and limits. Removal was confirmed by the tester. Read [SAFETY.md](../../SAFETY.md) before installation. Use only in a probe-only unsaved scratch patch.

The effect starts disabled. Keep Level at 80, observe Enable 0, then set Enable to 1 while the effect is fully on. A fresh instance already set to 1 cannot start until it observes 0 then 1. A running test aborts when Enable returns to 0, the bypass fade leaves 1, or valid coefficients resume after an invalid interval. Success and failure are permanent until reinitialisation; changing Enable cannot restart them.

The probe validates a fixed descriptor tuple, tests head prefixes of 16, 256, 4096 and 32768 bytes, tests the last 16 bytes, then checks joint retention. Every failure stops sample-memory accesses. The tested regions are `[0xc0378000, 0xc0380000)` and `[0xc0424430, 0xc0424440)`. The completed run demonstrated access and retention for those regions; it did not establish usability of the entire allocation or cross-instance isolation.

## Implementation

- `probe.c` adapts the host pointers and coefficients and emits finite FSK on both output channels
- `step.inc`, `packet.inc` and `transmitted.inc` are included directly into the leaf kernel and the desktop harness, with no function-call boundary
- `state.h` defines 184 bytes of per-instance state, including one 80-byte immutable packet, within a 188-byte allowance plus the generated guard
- Sample-memory operations are volatile 32-bit words, limited to sixteen accesses per callback
- Each packet has twenty words, sync `0xa71e5c3e`, version 1 and IEEE reflected CRC-32; CRC work is limited to one word per callback
- The kernel must contain no calls, including compiler-generated helpers; `allow_stack=true` does not waive this rule
- Bypass detection requires a callback that observes a fade below 1; a host suspension that hides the whole transition is not detectable through this coefficient

## Offline checks

Run from the Stomphacks root:

```sh
python3 tools-diagnostics/verify_memory_write.py build/memory-write-check
python3 -m unittest discover -s tools-diagnostics -p 'test_*.py' -v
ZOOM_TI_CGT=/Users/dev-vm/Applications/ti/ccs2101/ccs/tools/compiler/ti-cgt-c6000_8.5.0.LTS .venv/bin/python3 tools/zd2_make_effect.py effects/delay-memory-write-probe/manifest.json
```

The harness requires a C99 compiler supporting AddressSanitizer and UndefinedBehaviorSanitizer. It injects faults, checks boundaries and access budgets, and generates a synthetic PCM recording for the separate decoder. Desktop checks do not verify hardware memory access or DSP load. Inspect every build-report item and the generated kernel before hardware use.

## Recording and decoding

Start recording before selecting the fresh effect, then record 120 seconds after enabling. Use integer PCM WAV, preferably 44.1 kHz; one channel is sufficient. Keep the chain, Level, Enable and bypass unchanged during the run.

```sh
python3 tools-diagnostics/decode_memory_write.py capture.wav --json
```

The decoder preserves timestamps and packet sequence, separates search observations from physical transmissions, and reports missing intermediate evidence and resets. A missing terminal packet is inconclusive. A complete pass requires both region tests and the expected phase reports, with two distinct terminal packets.

Installation, exact preserved artifacts and session instructions are in the [Stage E results](../../../zoom-midiverb-planning/doc/delay-abi-investigation/stage-e-results.md). Never save the effect in a patch. Uninstall using the preserved local artifact at the end of the session and verify absence. If a transfer fails, keep the pedal powered and follow the rescue instructions in SAFETY.md.
