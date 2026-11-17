#!/usr/bin/env python3
"""Run desktop memory-probe checks and decode a synthetic complete PCM run.

Run from the repository root. This tool has no device or network access.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
import wave
import zlib
from decode_memory_write import RATE, SYMBOL, analyse, decode_samples
from decode_delay_memory import read_wav


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    head_callbacks = sum(2 * ((n + 7) // 8 + 2757 + (n + 15) // 16)
                         for n in (4, 64, 1024, 8192))
    tail_callbacks = 2 * (1 + 2757 + 1 + 512)
    joint_callbacks = 27563 + 1 + 512 + 1
    # Four gates each allow a residual packet plus two result packets;
    # final success allows another residual plus two terminal packets
    upper_seconds = ((head_callbacks + tail_callbacks + joint_callbacks + 32) * 16 +
                     15 * (20 * 32 * SYMBOL + 18 * 16)) / RATE
    assert upper_seconds < 100
    print(f'Conservative bound through two terminal packets: {upper_seconds:.6f} seconds')
    args.output.mkdir(parents=True, exist_ok=True)
    binary = args.output / 'test_engine'
    capture = args.output / 'packets.bin'
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-O2',
                    '-fsanitize=address,undefined',
                    'tools-diagnostics/memory-write-harness/test_engine.c',
                    '-o', str(binary)], check=True)
    result = subprocess.run([str(binary.resolve()), str(capture)], check=True,
                            text=True, capture_output=True)
    print(result.stdout, end='')
    (args.output / 'engine-results.txt').write_text(result.stdout)
    records = list(struct.iter_unpack('<21I', capture.read_bytes()))
    frames = records[-1][0] + SYMBOL
    pcm = bytearray(frames * 2)
    symbols = []
    for bit in range(2):
        mask, half = (15, 8) if bit else (31, 16)
        symbols.append(b''.join(struct.pack('<h', 6553 if n & mask < half else -6553)
                                for n in range(SYMBOL)))
    for end, *words in records:
        assert zlib.crc32(struct.pack('<18I', *words[1:19])) == words[19]
        pos = (end - 20 * 32 * SYMBOL) * 2
        for word in words:
            for bit in range(32):
                pcm[pos:pos + SYMBOL * 2] = symbols[(word >> bit) & 1]
                pos += SYMBOL * 2
    wav = args.output / 'synthetic-complete.wav'
    with wave.open(str(wav), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(pcm)
    report = analyse(decode_samples(read_wav(wav)))
    (args.output / 'synthetic-analysis.json').write_text(json.dumps(report, indent=2) + '\n')
    assert report['outcome'] == 'complete_pass', report
    assert report['physical_packets'] == len(records)
    print(f"PASS synthetic PCM: {len(records)} distinct packets, "
          f"{report['terminal_packets']} terminal packets, {report['outcome']}")
    print('Desktop evidence only; no hardware verification')
    return 0


if __name__ == '__main__':
    sys.exit(main())
