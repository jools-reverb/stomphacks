"""Offline transport and CRC tests (not a pedal test)."""
import random
import struct
import tempfile
import unittest
import wave
import zlib
from pathlib import Path
from decode_delay_memory import SYNC, SYMBOL, decode_samples, read_wav, RATE

def c_crc(data):
    crc = 0xffffffff
    for w in data:
        for _ in range(32):
            crc = (crc >> 1) ^ (0xedb88320 if ((crc ^ w) & 1) else 0)
            w >>= 1
    return crc ^ 0xffffffff

def generate(data, repeats=1):
    words = [SYNC, *data, c_crc(data)]
    return [0.2 if (n & (15 if (w >> b) & 1 else 31)) < (8 if (w >> b) & 1 else 16) else -0.2
            for _ in range(repeats) for w in words for b in range(32) for n in range(SYMBOL)]

class ProbeDecoderTests(unittest.TestCase):
    def test_crc(self):
        data = [0x800000, 0x900000, 0x910000, 65536, 0]
        self.assertEqual(c_crc(data), zlib.crc32(struct.pack("<5I", *data)))

    def test_noisy_offset_capture(self):
        data = [0x800000, 0x900000, 0x910000, 65536, 0]
        rng = random.Random(7)
        samples = [0] * 91 + generate(data, 2)
        samples = [x + rng.uniform(-0.02, 0.02) for x in samples]
        p = decode_samples(samples)[0]
        self.assertEqual([p[k] for k in ("pointer", "base", "end", "size_bytes", "status")], data)

    def test_null(self):
        self.assertEqual(decode_samples(generate([0,0,0,0,1]))[0]["status"], 1)

    def test_48k_stereo_wav(self):
        data = [0x800000,0x900000,0x910000,65536,0]
        src = [0.0]*103 + generate(data, 2)
        dst = []
        for k in range(len(src)*48000//RATE):
            pos = k*RATE/48000
            j = int(pos)
            f = pos-j
            dst.append(src[j]*(1-f)+src[min(j+1,len(src)-1)]*f)
        with tempfile.TemporaryDirectory() as temp:
            name = Path(temp)/"capture.wav"
            with wave.open(str(name),"wb") as out:
                out.setnchannels(2); out.setsampwidth(2); out.setframerate(48000)
                out.writeframes(b"".join(struct.pack("<hh",int(x*32767),int(x*32767)) for x in dst))
            p = decode_samples(read_wav(name))[0]
            self.assertEqual(p["size_bytes"], 65536)

if __name__ == "__main__":
    unittest.main()
