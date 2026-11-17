# Offline delay-memory decoder

Standard-library Python only; these tools never talk to the pedal.

```sh
python3 -m unittest discover -s tools-diagnostics -p 'test_*.py' -v
python3 tools-diagnostics/decode_delay_memory.py capture.wav
```

See [probe instructions](../effects/delay-memory-probe/README.md). Tests use synthetic WAV samples, not a Zoom capture.

## Bounded memory probe

`decode_memory_write.py` decodes the separate twenty-word MEM WRITE protocol. It retains chronological packets, deduplicates phase-search observations, checks progression and timing, and reports resets or missing evidence. Stage D decoding remains in `decode_delay_memory.py`.

```sh
python3 tools-diagnostics/decode_memory_write.py capture.wav --json
python3 tools-diagnostics/verify_memory_write.py build/memory-write-check
```

The second command runs the shared C logic under desktop sanitizers, injects memory and control faults, then generates and decodes a complete synthetic WAV. It has no device or network access. See the [effect README](../effects/delay-memory-write-probe/README.md) for build and recording instructions.
