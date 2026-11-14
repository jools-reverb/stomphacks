# Offline delay-memory decoder

Standard-library Python only; these tools never talk to the pedal.

```sh
python3 -m unittest discover -s tools-diagnostics -p 'test_*.py' -v
python3 tools-diagnostics/decode_delay_memory.py capture.wav
```

See [probe instructions](../effects/delay-memory-probe/README.md). Tests use synthetic WAV samples, not a Zoom capture.
