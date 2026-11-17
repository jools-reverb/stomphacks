#ifndef MEMORY_WRITE_STATE_H
#define MEMORY_WRITE_STATE_H
#include <stdint.h>
#define MW_SYNC 0xa71e5c3eu
#define MW_MAGIC 0x45315752u
#define MW_PACKET_FRAMES (20u * 32u * 256u)
#define MW_BASE 0xc0378000u
#define MW_END 0xc0424440u
#define MW_SIZE 705600u
#define MW_TAIL (MW_SIZE - 16u)
/* One immutable packet plus live state, including a pending first failure */
typedef struct {
    uint32_t packet[20];
    uint32_t magic, txpos, sequence, crc, crc_word;
    uint32_t elapsed, wait_frames, descriptor, base, end, size;
    uint32_t offset, length, cursor, highwater;
    uint32_t fail_offset, expected, observed;
    uint32_t phase, status, mask, detail, stage, gate, seen_zero, invalid;
} MemoryWriteState;
typedef char memory_write_state_fits[(sizeof(MemoryWriteState) <= 188) ? 1 : -1];
#define MW_FAIL(code) do { \
    if (s->status < 3u) { \
        s->status = ((code) == 7u) ? 5u : 4u; \
        s->detail = (s->detail & 0x00ffff00u) | (code); \
    } \
} while (0)
#define MW_PATTERN(index, tail, pattern) \
    (((tail) ? 0x3f000000u : 0x3e000000u) | \
     (((((index) * 0x00045d9fu + 0x0013579bu) & 0x007fffffu)) ^ \
      ((pattern) ? 0x007fffffu : 0u)))
/* Subtraction avoids end-address overflow, including protected extents */
#define MW_OVERLAP(a, n, b, m) \
    (((a) <= (b)) ? ((b) - (a) < (n)) : ((a) - (b) < (m)))
#endif
