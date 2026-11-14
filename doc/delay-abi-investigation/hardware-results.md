# Hardware results

**Initial tester-reported capture plus eight inspected Stage D recordings logged.** Append records without rewriting observations. The Stage D WAVs and current build report were directly inspected; missing evidence for Capture 1 remains unchanged.

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

## Stage D — eight allocation-policy captures; 2026-11-11

- **Measurement date:** 2026-11-11, based on the test session and supplied recording timestamps.
- **Device:** Zoom MS-70CDR+; firmware version not independently verified for this session.
- **Effect configurations:** Eight tests reported completed in the order below. `P` is MEM PROBE; stock neighbours are Delay (`DELAY_C`) and Hall (`HALL_C`). Chain order is input to output. Fresh probe selection and patch-reload actions are tester-reported, not independently visible in audio.
- **Probe:** `XXMEMTST.ZD2`, playground ID `07001040`, branch `feature/delay-memory-probe`, source commit `3c0c36a1fb3bcad3f62de588c1616d52829b7f34` plus the existing local manifest change removing `author: joolsa`. Source and manifest snapshots preserved with the build.
- **Build validation:** TI C6000 8.5.0 LTS build and every report check passed; generated kernel reviewed. Five decoder tests and the installer mock self-test passed. The latter reported skipping its unavailable-stock-corpus refusal check. Offline classification returned `DIY-OK`.
- **Artifact SHA-256:** ZD2 `fe96e02a124baca4be907e11c29d339ddede35b97c89de6f7869e963fc7610ce`; ZIC `eb4a58be135aff10331f27c6462d4a6a4f9926ac087cfab64e80cac5496b3319`.
- **Autosave, saved patch, backup, install readback:** Tester reported the prescribed safe-connect, backup, install and readback commands succeeded. Local byte comparison of the downloaded `installed.ZD2` against the preserved build reported no differences. Complete session logs were not independently inspected.
- **Recording:** Local `build/stage-d-2026-11-11/recordings/1.wav` through `8.wav`; all mono, 16-bit integer PCM, 44,100 Hz. Exact durations below. No digital full-scale clipping observed; absolute peaks ranged from 0.6412 to 0.6544 full scale.
- **Stock settings:** Enabled neighbours with wet Mix/Effect Level zero were prescribed, but exact parameter values and enabled states were not separately supplied. Results apply to the tester-reported configurations.
- **Decoder status:** All eight files contain one CRC-valid descriptor tuple. Independent decoding of each file's first and last three seconds returns the same tuple as its full capture. Observation counts include phase-search duplicates, **not** independent allocations.

### Decoder output (inspected recordings)

| Test / file | Reported configuration | Duration (s) | `instance[3]` | `base` | `end` (exclusive) | CRC-valid observations (full / first 3 s / last 3 s) |
|---|---|---:|---|---|---|---|
| D1 / `1.wav` | P alone | 18.260204 | `0xc02758a8` | `0xc0378000` | `0xc0424440` | 192 / 15 / 30 |
| D2 / `2.wav` | Delay → P | 11.201338 | `0xc02758b4` | `0xc0424440` | `0xc04d0880` | 113 / 23 / 15 |
| D3 / `3.wav` | P → Delay | 12.037256 | `0xc02758a8` | `0xc0378000` | `0xc0424440` | 99 / 8 / 30 |
| D4 / `4.wav` | Hall → P | 11.851474 | `0xc02758b4` | `0xc0424440` | `0xc04d0880` | 105 / 15 / 15 |
| D5 / `5.wav` | P → Hall | 10.829819 | `0xc02758a8` | `0xc0378000` | `0xc0424440` | 105 / 15 / 15 |
| D6 / `6.wav` | P₁ → P₂; P₂ observed | 12.315896 | `0xc02758b4` | `0xc0424440` | `0xc04d0880` | 111 / 7 / 15 |
| D7 / `7.wav` | P recreated alone after chain edits | 10.736916 | `0xc02758a8` | `0xc0378000` | `0xc0424440` | 120 / 30 / 30 |
| D8 / `8.wav` | P recreated alone after patch reload | 10.736916 | `0xc02758a8` | `0xc0378000` | `0xc0424440` | 93 / 15 / 7 |

Common decoded fields for every recording:

```text
status: descriptor read
size_bytes: 705600 (689.06 KiB)
span: 705600 bytes
span matches size: True
4-byte pointer/base/end alignment: True
4-byte size alignment: True
usable from 8-byte alignment: 705600 bytes
32 KiB fits: True
```

### Interpretation

- Every configuration reports **705,600 bytes** (`0x000ac440`, **689.06 KiB**), enough for both the 32 KiB int16 and 64 KiB float32 targets.
- First-position/solo captures match Capture 1. Second-position captures report a descriptor 12 bytes higher and a region 705,600 bytes higher; its base equals the first region's exclusive end.
- The pattern supports fixed-size, position-associated regions for the first two chain positions in these tested configurations. It does not establish the host's policy for all positions, settings or effect classes.
- Delay, Hall and an upstream custom probe did not reduce the observed second-position capacity. D6 reports only P₂: its output replaces P₁'s telemetry, so simultaneous independent allocations were not measured.
- Recreation and patch reload returned the original reported addresses. Address reuse does not prove sample-content persistence.
- No inconsistent descriptor, contradictory tuple or insufficient-capacity result requires a repeat. The planned Stage D matrix is complete with the metadata limitations above.

**Evidence level:** Directly inspected CRC-valid WAV telemetry and local build/readback comparison, with tester-reported physical configurations and actions. The probe repeats a snapshot captured at initialisation; repeated packets do not measure live descriptor changes. No sample-buffer reads or writes were attempted. **Writability, persistence, isolation and accessibility of the sample region remain unverified.** Stage E requires a separate bounded-write plan and memory-map safety review.

**Cleanup:** Following reconnection and the corrected uninstall command using the preserved local artifact path, the tester reported “All absent” on 2026-11-11. Absence verification is tester-reported. No further Stage D device operations are needed.

Recording SHA-256 hashes and detailed evidence are retained in the [planning results](../../../zoom-midiverb-planning/doc/delay-abi-investigation/stage-d-results.md); local machine-readable decoder output is `build/stage-d-2026-11-11/analysis.json`. Private backups and generated binaries are not committed.

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
