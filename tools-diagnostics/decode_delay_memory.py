#!/usr/bin/env python3
"""Decode 7-word two-tone FSK telemetry sent by the MS-70CDR+ delay probe."""
import argparse
import math
import struct
import sys
import wave
import zlib

RATE, SYMBOL, SYNC, WORDS = 44100, 256, 0xA71D5C3E, 7
STATUS = {0: "descriptor read", 1: "null instance[3]", 2: "unaligned instance[3]"}


def resample(audio, rate):
    if rate == RATE: return audio
    if not audio: return []
    result = []
    for n in range(len(audio) * RATE // rate):
        p = n * rate / RATE
        j = int(p)
        f = p - j
        result.append(audio[j] * (1 - f) + audio[min(j + 1, len(audio) - 1)] * f)
    return result


def read_wav(path):
    with wave.open(str(path), "rb") as f:
        channels, width, rate = f.getnchannels(), f.getsampwidth(), f.getframerate()
        if channels < 1 or width not in (2, 3, 4) or rate <= 0 or f.getcomptype() != "NONE":
            raise ValueError("need uncompressed 16/24/32-bit integer PCM WAV")
        data = f.readframes(f.getnframes())
    if width == 2:
        values = (v[0] / 32768.0 for v in struct.iter_unpack("<h", data))
    elif width == 4:
        values = (v[0] / 2147483648.0 for v in struct.iter_unpack("<i", data))
    else:
        def samples24():
            for k in range(0, len(data), 3):
                v = data[k] | data[k + 1] << 8 | data[k + 2] << 16
                if v & 0x800000: v -= 0x1000000
                yield v / 8388608.0
        values = samples24()
    return resample([v for i, v in enumerate(values) if i % channels == 0], rate)


def decode_samples(samples):
    """Search phase offsets and keep only CRC-verified complete packets."""
    count = WORDS * 32
    if len(samples) < count * SYMBOL: return []
    refs = [([math.cos(2 * math.pi * c * n / SYMBOL) for n in range(SYMBOL)],
             [math.sin(2 * math.pi * c * n / SYMBOL) for n in range(SYMBOL)])
            for c in (8, 16)]
    syncbits = [(SYNC >> k) & 1 for k in range(32)]
    found = {}
    for phase in range(0, SYMBOL, 16):
        bit_count = (len(samples) - phase) // SYMBOL
        bits = []
        for i in range(bit_count):
            window = samples[phase + SYMBOL * i:phase + SYMBOL * (i + 1)]
            energies = []
            for cs, sn in refs:
                real = sum(v * b for v, b in zip(window, cs))
                imag = sum(v * b for v, b in zip(window, sn))
                energies.append(real * real + imag * imag)
            bits.append(int(energies[1] > energies[0]))
        for start in range(bit_count - count + 1):
            if bits[start:start + 32] != syncbits: continue
            words = [sum(bits[start + w * 32 + k] << k for k in range(32))
                     for w in range(WORDS)]
            if zlib.crc32(struct.pack("<5I", *words[1:6])) != words[6]:
                continue
            packet = tuple(words[1:6])
            found[packet] = found.get(packet, 0) + 1
    return [dict(pointer=p[0], base=p[1], end=p[2], size_bytes=p[3],
                 status=p[4], observations=n)
            for p, n in sorted(found.items(), key=lambda item: -item[1])]


def describe(p):
    base, end, size = p["base"], p["end"], p["size_bytes"]
    fields = [f"status: {STATUS.get(p['status'], 'unknown')}",
              f"instance[3]: 0x{p['pointer']:08x}",
              f"base: 0x{base:08x}", f"end (exclusive): 0x{end:08x}",
              f"size_bytes: {size} ({size/1024:.2f} KiB)",
              f"CRC-valid observations: {p['observations']}"]
    if p["status"] == 0:
        usable = max(0, end - ((base + 7) & ~7))
        fields += [f"span: {end-base} bytes", f"span matches size: {end>base and end-base==size}",
                   f"4-byte pointer/base alignment: {(p['pointer']&3)==0 and (base&3)==0}",
                   f"usable from 8-byte alignment: {usable} bytes",
                   f"32 KiB fits: {usable>=32768}",
                   "Values alone DO NOT prove writable, independent or persistent memory"]
        if not (end > base and end-base == size and (base&3) == 0):
            fields.append("WARNING: inconsistent descriptor; DO NOT access buffer")
    return "\n".join(fields)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wav", help="uncompressed PCM WAV")
    a = ap.parse_args()
    try:
        packets = decode_samples(read_wav(a.wav))
    except (OSError, EOFError, ValueError, wave.Error) as error:
        ap.error(str(error))
    if not packets:
        print("No CRC-valid packet found", file=sys.stderr)
        return 1
    for p in packets:
        print(describe(p), "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
