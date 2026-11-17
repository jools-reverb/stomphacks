"""Transport faults and chronological evidence checks for the memory probe."""
import random
import struct
import tempfile
import unittest
import wave
import zlib
from pathlib import Path
from decode_memory_write import (SYNC, BASE, END, SIZE, RATE, SYMBOL,
                                 decode_samples, analyse, packet_record)
from decode_delay_memory import read_wav


def words(sequence=0, phase=0, status=0, elapsed=0, **changes):
    p = [SYNC, 1, sequence, phase, status, (1 << phase) - 1 if status in (2, 3) else 0,
         0, elapsed, 0xC02758A8, BASE, END, SIZE, 0, 16 if phase > 1 else 0,
         16 if phase > 1 else 0,
         32768 if phase >= 3 else 16 if phase == 2 else 0, 0xFFFFFFFF, 0, 0]
    for key, value in changes.items():
        p[int(key)] = value
    return p + [zlib.crc32(struct.pack('<18I', *p[1:]))]


def generate(p):
    return [0.2 if (n & (15 if (w >> b) & 1 else 31)) <
            (8 if (w >> b) & 1 else 16) else -0.2
            for w in p for b in range(32) for n in range(SYMBOL)]


def sequence():
    packets = []
    for phase in range(1, 6):
        for _ in range(2):
            seq = len(packets)
            # Idle gaps between phases allow the documented retention waits
            elapsed = (phase * 20 * RATE if phase == 5 else
                       (phase * 20 + seq % 2 * 4) * RATE)
            p = words(seq, phase, 3 if phase == 5 else 2, elapsed)
            packets.append(packet_record(p, (phase * 20 + seq % 2 * 4) * RATE))
    return dict(packets=packets, physical_packets=len(packets), crc_valid_observations=10)


class MemoryWriteDecoderTests(unittest.TestCase):
    def test_sequence(self):
        self.assertEqual(analyse(sequence())['outcome'], 'complete_pass')

    def test_noise_offsets_and_deduplication(self):
        rng = random.Random(44)
        p = words()
        samples = [0.0] * 91 + generate(p) + generate(words(1)) + [0.0] * 256
        samples = [x + rng.uniform(-0.03, 0.03) for x in samples]
        result = decode_samples(samples)
        self.assertEqual(result['physical_packets'], 2)
        self.assertGreater(result['crc_valid_observations'], 2)
        self.assertEqual([p['sequence'] for p in result['packets']], [0, 1])
        self.assertLess(abs(result['packets'][0]['sample'] - 91), SYMBOL)

    def test_corrupt_crc_wrong_version_and_truncation(self):
        p = words()
        p[-1] ^= 1
        result = decode_samples(generate(p))
        self.assertFalse(result['packets'])
        self.assertGreater(result['crc_rejected_observations'], 0)
        p = words(**{'1': 2})
        result = analyse(decode_samples(generate(p)))
        self.assertIn('unsupported protocol version', result['issues'])
        self.assertFalse(decode_samples(generate(words())[:-512])['packets'])
        self.assertGreater(decode_samples(generate(words())[:-512])['truncated_sync_observations'], 0)

    def test_all_statuses_and_errors(self):
        for status in range(6):
            p = words(status=status)
            result = decode_samples(generate(p))
            self.assertEqual(result['packets'][0]['status'], status)
        for error in range(8):
            p = words(status=5 if error == 7 else 4, **{'6': error})
            result = analyse(decode_samples(generate(p)))
            self.assertEqual(result['packets'][0]['detail'], error)
            self.assertEqual(result['outcome'], 'not_pass')

    def test_48k_pcm(self):
        source = [0.0] * 103 + generate(words()) + [0.0] * 256
        pcm = bytearray()
        for k in range(len(source) * 48000 // RATE):
            pos = k * RATE / 48000
            j = int(pos)
            f = pos - j
            x = source[j] * (1 - f) + source[min(j + 1, len(source) - 1)] * f
            pcm.extend(struct.pack('<h', int(x * 32767)))
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'test.wav'
            with wave.open(str(path), 'wb') as out:
                out.setnchannels(1)
                out.setsampwidth(2)
                out.setframerate(48000)
                out.writeframes(pcm)
            self.assertEqual(decode_samples(read_wav(path))['physical_packets'], 1)

    def test_missing_reset_and_terminal_freeze(self):
        result = sequence()
        del result['packets'][3]
        report = analyse(result)
        self.assertEqual(report['outcome'], 'terminal_pass_with_missing_evidence')
        self.assertIn('packet sequence gap', report['missing_evidence'])
        result = sequence()
        result['packets'][5]['sequence'] = 0
        self.assertEqual(analyse(result)['outcome'], 'not_pass')
        result = sequence()
        result['packets'][5]['elapsed'] = 1
        self.assertEqual(analyse(result)['outcome'], 'not_pass')
        result = sequence()
        result['packets'][-1]['observed'] = 4
        self.assertIn('terminal record changed', analyse(result)['issues'])

    def test_bad_mask_descriptor_timing_and_progress(self):
        for field, value in [('mask', 0), ('descriptor', 0), ('base', BASE + 4),
                             ('elapsed', 1), ('highwater', 16), ('version', 2),
                             ('detail', 0xFF000000), ('phase', 2)]:
            with self.subTest(field=field):
                result = sequence()
                result['packets'][-1][field] = value
                self.assertEqual(analyse(result)['outcome'], 'not_pass')
        self.assertEqual(analyse(dict(packets=[]))['outcome'], 'not_pass')


if __name__ == '__main__':
    unittest.main()
