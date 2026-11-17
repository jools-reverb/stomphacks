# MS-70CDR+ delay-memory descriptor probe

**Experimental: C kernel not yet built with TI cl6x or run on a pedal.** Read [SAFETY.md](../../SAFETY.md) completely before any hardware testing. The original Stomphacks gain effect remains unchanged.

## Behaviour

On the first audio callback after valid initialisation, read `instance[3]`. A null pointer produces status 1; a non-four-byte-aligned pointer produces status 2. Otherwise **immediately read the first three 32-bit words** and report status 0. There is no separate pointer-only binary. The fields are interpreted as base address, exclusive end and size in **bytes**. This test does not access the buffer pointed to by base, nor write to any delay sample memory. A non-null aligned pointer could still be unmapped and cause a DSP crash.

A 7-word packet is repeated on both output channels. Each 32-bit word is transmitted least-significant-bit first:
1. Sync `0xA71D5C3E`
2. `instance[3]` descriptor address
3. `base`
4. `end`
5. `size_bytes`
6. Status (0 read, 1 null, 2 misaligned)
7. IEEE CRC-32 over words 2–6 in little-endian byte order

Each bit is a 256-sample square-wave symbol at 44.1 kHz: a zero uses a 32-sample period (1,378.125 Hz), a one a 16-sample period (2,756.25 Hz). Packet duration is approximately 1.30 seconds, repeated continuously. The existing gain example's init guard and finite audio-output clamp patterns are retained.

## Off-device build

First follow [Stomphacks building instructions](../../docs/building-effects.md) to install TI C6000 Code Generation Tools 8.5.0, Python requirements and the `zoom-zt2/` checkout. Then, from the repository root:

```sh
.venv/bin/python3 tools/zd2_make_effect.py effects/delay-memory-probe/manifest.json
```

Inspect **every** check in `effects/delay-memory-probe/build/REPORT.md`. The manifest uses the documented `kernel_section=text` and `allow_stack=true` options because telemetry is larger than the ordinary gain effect. They have not yet been compiled/validated for this kernel.

Identity: playground SFX ID `07001040`, filename `XXMEMTST`. Ensure neither identity collides with something already installed, and never distribute saved patches referring to this playground ID.

## Decode

```sh
python3 -m unittest discover -s tools-diagnostics -p 'test_*.py' -v
python3 tools-diagnostics/decode_delay_memory.py capture.wav
```

Capture four or more seconds of clean output from an enabled probe to an **uncompressed integer PCM WAV**, ideally at 44.1 kHz (48 kHz is supported). Use either stereo channel. Keep Level at the default of 80 and avoid clipping. The Python decoder checks sync and CRC before accepting a packet. A zero or inconsistent capacity must not trigger speculative memory accesses.

## Device safety

Follow [SAFETY.md](../../SAFETY.md): use `tools-pedal/safe_connect.py` and confirm autosave OFF and a stock-only saved patch; back up everything using `tools-pedal/backup_pedal.py`; run `tools-pedal/pedal_diy.py classify effects/delay-memory-probe/build/XXMEMTST.ZD2` before installing. Use `tools-pedal/pedal_diy.py install` with the companion ZIC, then follow the documented readback/byte comparison steps. Test **only in an unsaved scratch patch**; never save experimental code into a patch. Uninstall after the session.

**If any file transfer stalls, DO NOT POWER-CYCLE.** Keep the pedal powered and follow `pedal_diy.py rescue` as specified in SAFETY.md; recovery is not guaranteed.

Log observations using [hardware-results.md](../../doc/delay-abi-investigation/hardware-results.md), and follow the [hardware test plan](https://github.com/jools-reverb/zoom-midiverb-planning/blob/main/doc/delay-abi-investigation/hardware-test-plan.md). No hardware outcomes have been collected.
