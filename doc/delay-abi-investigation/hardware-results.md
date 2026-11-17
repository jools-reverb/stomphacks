# Hardware results

**Initial tester-reported capture, eight inspected Stage D recordings and the Stage E bounded-write/retention pass logged.** Append records without rewriting observations. The Stage D WAVs and current build report were directly inspected; missing evidence for Capture 1 remains unchanged.

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

## Stage E — off-device preparation

At off-device preparation, no Stage E installation or hardware recording had been performed. The separate MEM WRITE probe (`XXMEMWR`, ID `07001050`) was implemented and built; sample-memory accessibility, writability, retention and isolation were then hardware-unverified.

- TI C6000 8.5.0 LTS build and all report checks pass, with no calls in the audio kernel
- Shared C logic passes 41 desktop fault-injection scenarios with address and undefined-behaviour sanitizers, a 184-byte state and a maximum of sixteen sample-memory accesses per callback
- Twelve decoder tests pass, including the existing Stage D suite; a synthetic complete recording decodes nineteen physical packets and three terminal PASS packets
- Installer mock self-test passes, with its stock-corpus refusal subcheck skipped because its expected directory is absent; local classification is DIY-OK
- Preserved ZD2 SHA-256: `330a49b902cf9303625327bab4b65b8794c5fcd92366aa1c2ffb33a16732b542`
- Preserved ZIC SHA-256: `55371ce4047f5debfebd20837d1ccc6fc7aac88fff779c596e33257b98587922`
- Exact artifacts, source snapshots and checksums are under ignored `build/stage-e-2026-11-14/artifact/`

Detailed off-device evidence and subsequent hardware results are in [Stage E results](../../../zoom-midiverb-planning/doc/delay-abi-investigation/stage-e-results.md). These are preparation results, not a Stage E hardware pass.

## Stage E — E1 bounded-write and retention capture; 2026-11-14

- **Outcome:** all five phases pass; decoder reports `complete_pass`, with no issues or missing required evidence
- **Device/session:** tester's MS-70CDR+; the retained cleanup-session identity file reports firmware 1.20, with no separately retained identity snapshot from recording time
- **Artifact:** MEM WRITE, `XXMEMWR.ZD2`, ID `07001050`, source/tool commit `1ccc215d1eafbab17568cfecddecfa9a68a2cb0f`; upload SHA-256 `330a49b902cf9303625327bab4b65b8794c5fcd92366aa1c2ffb33a16732b542`
- **Installation:** local operation log records install DONE at 22:41:49; independently compared `readback/installed.ZD2` is byte-identical to the preserved upload
- **Conditions:** the prescribed setup was a fresh probe-only unsaved scratch patch, Level 80 and Enable 0 then 1; exact physical settings, initial autosave confirmation and backup completion were not independently audited from complete console evidence
- **Recording:** `build/stage-e-2026-11-14/recordings/E1-bounded-write.wav`, mono 16-bit integer PCM, 44100 Hz, 8280475 frames, 187.765873 seconds; peak 0.64855957 full scale with no full-scale endpoint samples
- **WAV SHA-256:** `aad65c3172ccea7123b20efd089a73f17fd3cd9018969574b8a1e29a1f5b567c`
- **JSON SHA-256:** `096cad2f75fd3a7bd8034762c7da607965ece6e51a9e8e5eeef9bfd23f393f3a`; an independent full-WAV decode exactly matches the tester's JSON content
- **Descriptor:** pointer `0xc02758a8`, base `0xc0378000`, exclusive end `0xc0424440`, size 705600 bytes; no pointer/field change detected during the probe's active checks
- **Tested head:** `[0xc0378000, 0xc0380000)`, 32768 bytes, reached through 16-, 256-, 4096- and 32768-byte prefixes with both immediate and delayed pattern comparisons
- **Tested tail:** `[0xc0424430, 0xc0424440)`, 16 bytes, both patterns passed while the entire head B pattern remained unchanged
- **Joint retention:** at least ten seconds without writes, followed by successful tail/head/tail verification
- **Telemetry:** 44 distinct consecutive packets, sequences 0–43; two reports for each intermediate phase and 27 terminal PASS packets, mask `0x1f`, head high-water 32768, error zero, failure offset `0xffffffff`
- **Timing:** terminal completion at 2430112 processed frames, 55.104580 seconds after Enable; first terminal transmission begins around 83.886803 seconds into the recording
- **Search counts:** 650 CRC-valid observations, nine rejected and fifteen truncated-sync observations; search observations are not physical packet counts or memory failures
- **Cleanup:** tester explicitly confirmed removal; local operation log records uninstall DONE at 23:08:46 using the preserved local artifact; no captured ABSENT response was available for independent inspection

**Evidence level:** directly inspected WAV, independent decoder output and installed readback comparison, with tester-confirmed removal supported by the local uninstall log. Successful access and retention are established only for these two tested regions in this run. Frozen terminal repeats do not extend retention measurements. Cross-instance isolation, the untested middle, int16-ring correctness and measured DSP load remain unresolved. No repeat is required for this bounded experiment.

Detailed phase data, evidence limitations and retained artifact references are in [Stage E results](../../../zoom-midiverb-planning/doc/delay-abi-investigation/stage-e-results.md). Stage F has not started.

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
