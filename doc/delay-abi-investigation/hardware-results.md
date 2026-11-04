# Hardware results

**One user-reported on-pedal descriptor measurement logged.** Append records without rewriting observations. No WAV file or build report has been independently inspected.

## Capture 1 — probe only; report received 2026-11-04

- **Measurement date:** Not supplied (decoder output reported 2026-11-04).
- **Device:** Zoom MS-70CDR+; firmware version not supplied.
- **Effect configuration:** One delay-memory probe; **no other effects** in the chain (confirmed by the tester).
- **Probe:** `XXMEMTST` (playground ID `07001040`); development branch `feature/delay-memory-probe`. Exact locally installed build commit and `build/REPORT.md` not supplied.
- **Autosave, saved patch, backup, install readback:** Not independently confirmed in the report.
- **Recording:** WAV filename, sample format and duration not supplied; decoded output supplied as text.
- **Decoder status:** Descriptor read; 135 CRC-valid observations. These are repeated decodes of transmitted data, **not** 135 independent measurements.

### Decoder output (as provided)

```text
status: descriptor read
instance[3]: 0xc02758a8
base: 0xc0378000
end (exclusive): 0xc0424440
size_bytes: 705600 (689.06 KiB)
CRC-valid observations: 135
span: 705600 bytes
span matches size: True
4-byte pointer/base/end alignment: True
4-byte size alignment: True
usable from 8-byte alignment: 705600 bytes
32 KiB fits: True
Values alone DO NOT prove writable, independent or persistent memory
```

### Interpretation

- Descriptor address: `0xc02758a8`.
- Buffer range: `[0xc0378000, 0xc0424440)` (exclusive end).
- Reported size: **705,600 bytes** (`0x000ac440`, **689.06 KiB**).
- `end - base = size_bytes = 705600`; the reported alignment checks pass, and the buffer base is already 8-byte aligned.
- Reported usable capacity is **larger than 32,768 bytes** required for a 16,384-sample int16 ring (and larger than 65,536 bytes for the float32 equivalent).
- The reported byte count is also `44100 × 4 × 4`, equivalent in size to four seconds of mono float32 audio at 44.1 kHz; that arithmetic does **not** establish the host's allocation policy.

**Evidence level:** Tester-provided, CRC-validated decoder output for a single-effect configuration. No direct buffer reads/writes were attempted. **Writability, persistence, isolation, accessibility of the sample region, and allocation policy across configurations remain unverified.** A subsequent different effect-chain configuration is needed before drawing allocation-policy conclusions.

## Capture template

- Date, firmware version, model:
- Git branch / commit and generated build REPORT:
- Saved patch stock-only and autosave off confirmed:
- Diagnostic effect filename / ID:
- Effect-chain order and neighbors:
- Recording filename and sample format:
- Number of CRC-confirmed decodes:
- Descriptor read status:
- `instance[3]` address (hex):
- `base` address (hex):
- `end` (exclusive, hex):
- `size_bytes` (decimal and hex):
- `end-base` match and alignment:
- Usable 8-byte-aligned bytes:
- Replication after patch/chain changes:
- Anomalies / recovery actions:
- Conclusion and remaining uncertainties:

Do not commit private pedal backups or proprietary Zoom binaries.
