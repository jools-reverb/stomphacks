#!/usr/bin/env python3
"""Decode bounded-memory probe PCM recordings, preserving packet chronology."""
import argparse
from array import array
import json
import math
import struct
import sys
import zlib

from decode_delay_memory import read_wav

RATE, SYMBOL, SYNC, WORDS = 44100, 256, 0xA71E5C3E, 20
BASE, END, SIZE = 0xC0378000, 0xC0424440, 705600
STATUS = {0: 'idle', 1: 'running', 2: 'phase-pass', 3: 'terminal-pass',
          4: 'failure', 5: 'abort'}
ERROR = {0: 'none', 1: 'descriptor pointer', 2: 'descriptor fields',
         3: 'bounds/alignment/overlap', 4: 'descriptor changed',
         5: 'data mismatch', 6: 'state invariant', 7: 'user abort'}
FIELDS = ('version', 'sequence', 'phase', 'status', 'mask', 'detail', 'elapsed',
          'descriptor', 'base', 'end', 'size', 'offset', 'length', 'cursor',
          'highwater', 'fail_offset', 'expected', 'observed')


def packet_record(words, sample, observations=1):
    result = dict(zip(FIELDS, words[1:19]))
    result.update(time_seconds=sample / RATE, sample=sample,
                  crc_valid_observations=observations,
                  status_name=STATUS.get(words[4], 'unknown'),
                  error_name=ERROR.get(words[6] & 255, 'unknown'))
    return result


def decode_samples(samples):
    """Rolling tone correlations at 16-sample search spacing; no dependencies.

    Correlation phase is global; squared magnitudes are phase invariant.
    Multiple phase-search hits within a symbol are one physical transmission.
    """
    refs = [(math.cos(2 * math.pi * n / 32), math.sin(2 * math.pi * n / 32),
             math.cos(2 * math.pi * n / 16), math.sin(2 * math.pi * n / 16))
            for n in range(32)]
    prefix = [array('d', [0]) for _ in range(4)]
    a = b = c = d = 0.0
    for n, value in enumerate(samples):
        r = refs[n & 31]
        a += value * r[0]
        b += value * r[1]
        c += value * r[2]
        d += value * r[3]
        if n & 15 == 15:
            prefix[0].append(a)
            prefix[1].append(b)
            prefix[2].append(c)
            prefix[3].append(d)
    syncbits = [(SYNC >> k) & 1 for k in range(32)]
    hits, bad, truncated = [], [], []
    for phase in range(16):
        bits = []
        for i in range(phase, len(prefix[0]) - 16, 16):
            x = prefix[0][i + 16] - prefix[0][i]
            y = prefix[1][i + 16] - prefix[1][i]
            u = prefix[2][i + 16] - prefix[2][i]
            v = prefix[3][i + 16] - prefix[3][i]
            bits.append(int(u * u + v * v > x * x + y * y))
        for start in range(len(bits) - 31):
            if bits[start:start + 32] != syncbits:
                continue
            sample = phase * 16 + start * SYMBOL
            if start + WORDS * 32 > len(bits):
                truncated.append(sample)
                continue
            words = tuple(sum(bits[start + w * 32 + k] << k for k in range(32))
                          for w in range(WORDS))
            if zlib.crc32(struct.pack('<18I', *words[1:19])) != words[19]:
                bad.append(sample)
                continue
            hits.append((sample, words))
    groups = []
    for sample, words in sorted(hits):
        if groups and sample - groups[-1][0][0] < SYMBOL and words == groups[-1][0][1]:
            groups[-1].append((sample, words))
        else:
            groups.append([(sample, words)])
    packets = []
    for group in groups:
        sample, words = group[len(group) // 2]
        packets.append(packet_record(words, sample, len(group)))
    return dict(packets=packets, crc_valid_observations=len(hits),
                physical_packets=len(packets), crc_rejected_observations=len(bad),
                truncated_sync_observations=len(truncated),
                duration_seconds=len(samples) / RATE)


def analyse(decoded):
    packets = decoded['packets']
    issues, missing = [], []
    pass_counts = {p: 0 for p in range(1, 5)}
    terminal_count = 0
    previous = None
    descriptor = None
    last_phase_pass = {}
    for p in packets:
        phase, status, mask, detail = (p[k] for k in ('phase', 'status', 'mask', 'detail'))
        error, operation, pattern = detail & 255, detail >> 8 & 255, detail >> 16 & 255
        if p['version'] != 1:
            issues.append('unsupported protocol version')
        if status not in STATUS or phase not in range(6) or error not in ERROR:
            issues.append('unknown phase/status/error')
        if detail >> 24 or operation > 3 or pattern > 1 or mask & ~31:
            issues.append('invalid detail or pass mask')
        if p['highwater'] not in (0, 16, 256, 4096, 32768):
            issues.append('invalid high-water mark')
        if status in (4, 5):
            issues.append(f"{STATUS[status]}: {ERROR.get(error, 'unknown')}")
        elif error:
            issues.append('error attached to non-failure status')
        if error == 5:
            if p['expected'] == p['observed'] or p['fail_offset'] == 0xFFFFFFFF:
                issues.append('invalid mismatch record')
        elif p['fail_offset'] != 0xFFFFFFFF or p['expected'] or p['observed']:
            issues.append('unexpected mismatch fields')
        if previous:
            if p['sequence'] <= previous['sequence'] or p['elapsed'] < previous['elapsed']:
                issues.append('sequence/elapsed reset: separate runs must not be merged')
            elif p['sequence'] != previous['sequence'] + 1:
                missing.append('packet sequence gap')
            if phase < previous['phase'] or mask & previous['mask'] != previous['mask']:
                issues.append('phase/mask regression')
            if p['highwater'] < previous['highwater']:
                issues.append('high-water regression')
            if previous['status'] >= 3 and (status != previous['status'] or
                    any(p[k] != previous[k] for k in
                        ('phase', 'mask', 'detail', 'elapsed', 'descriptor', 'base', 'end',
                         'size', 'offset', 'length', 'cursor', 'highwater', 'fail_offset',
                         'expected', 'observed'))):
                issues.append('terminal record changed')
            delta = (p['elapsed'] - previous['elapsed']) / RATE
            # Snapshot timing differs from detected sync by at most search jitter
            if delta > p['time_seconds'] - previous['time_seconds'] + 0.03:
                issues.append('elapsed frames exceed recording time')
        if phase >= 1 and status in (1, 2, 3):
            # Phase 1 running is reserved for validation before acceptance
            if phase > 1 or status in (2, 3):
                if (p['base'], p['end'], p['size']) != (BASE, END, SIZE):
                    issues.append('accepted descriptor tuple differs from Stage D')
                if not p['descriptor'] or p['descriptor'] & 3:
                    issues.append('invalid accepted descriptor pointer')
                if descriptor is None:
                    descriptor = p['descriptor']
                elif descriptor != p['descriptor']:
                    issues.append('descriptor changed')
            required = (1 << (phase - 1)) - 1
            if status in (2, 3):
                required |= 1 << (phase - 1)
            if mask != required:
                issues.append('phase/pass-mask inconsistency')
        if phase > 1 and status in (1, 2, 3):
            if (p['offset'] not in (0, SIZE - 16) or
                    p['length'] not in (16, 256, 4096, 32768) or
                    p['offset'] == SIZE - 16 and p['length'] != 16 or
                    p['cursor'] > p['length'] or p['cursor'] & 3):
                issues.append('invalid region/cursor')
        if status == 0 and (phase or mask or p['elapsed']):
            issues.append('invalid idle record')
        if status == 2:
            if phase not in pass_counts:
                issues.append('unexpected phase-pass phase')
            else:
                pass_counts[phase] += 1
                minimum = {1: 0, 2: 2, 3: 8, 4: 10}[phase] * RATE
                if p['elapsed'] < minimum:
                    issues.append('retention timing too short')
                expected_highwater = {1: 0, 2: 16, 3: 32768, 4: 32768}[phase]
                if p['highwater'] != expected_highwater:
                    issues.append('phase-pass high-water mismatch')
                last_phase_pass[phase] = p['elapsed']
        if status in (2, 3) and phase in (2, 3, 4, 5) and phase - 1 in last_phase_pass:
            # The last prior result snapshot precedes its full transmission;
            # only after that packet may the next phase's waits begin
            waits = {2: 2, 3: 6, 4: 2, 5: 10}[phase]
            minimum_delta = waits * RATE + WORDS * 32 * SYMBOL
            if p['elapsed'] - last_phase_pass[phase - 1] < minimum_delta:
                issues.append('inter-phase retention timing too short')
        if status == 3:
            terminal_count += 1
            if phase != 5 or mask != 31 or p['highwater'] != 32768:
                issues.append('invalid terminal PASS')
            if p['elapsed'] < 20 * RATE:
                issues.append('joint retention timing too short')
        previous = p
    for phase, count in pass_counts.items():
        if count < 2:
            missing.append(f'phase {phase}: {count}/2 complete result packets observed')
    if terminal_count < 2:
        missing.append(f'terminal PASS: {terminal_count}/2 distinct packets observed')
    if not terminal_count:
        issues.append('no terminal PASS: inconclusive')
    outcome = ('not_pass' if issues else
               'terminal_pass_with_missing_evidence' if missing else 'complete_pass')
    return dict(decoded, outcome=outcome, issues=list(dict.fromkeys(issues)),
                missing_evidence=list(dict.fromkeys(missing)),
                phase_pass_packets=pass_counts, terminal_packets=terminal_count,
                evidence_limit='Only 32 KiB head and 16-byte tail access/retention; '
                               'no cross-instance isolation, middle-span or int16 proof')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('wav')
    parser.add_argument('--json', action='store_true', help='emit full chronological evidence')
    args = parser.parse_args()
    try:
        result = analyse(decode_samples(read_wav(args.wav)))
    except (OSError, ValueError, EOFError) as error:
        parser.error(str(error))
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"{result['outcome']}: {result['physical_packets']} physical packets, "
              f"{result['crc_valid_observations']} CRC-valid search observations")
        for p in result['packets']:
            print(f"{p['time_seconds']:8.3f}s seq={p['sequence']} phase={p['phase']} "
                  f"{p['status_name']} mask=0x{p['mask']:02x} elapsed={p['elapsed']/RATE:.3f}s "
                  f"region={p['offset']}+{p['length']} cursor={p['cursor']} "
                  f"head={p['highwater']} error={p['error_name']}")
        for message in result['issues'] + result['missing_evidence']:
            print(message)
        print(result['evidence_limit'])
    return 0 if result['outcome'] == 'complete_pass' else 1


if __name__ == '__main__':
    sys.exit(main())
