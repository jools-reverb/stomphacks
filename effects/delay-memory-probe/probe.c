/* Delay-memory descriptor probe for Zoom MS-70CDR+.
 * Experimental, hardware-UNTESTED. No delay-buffer reads or writes.
 *
 * Uses the Stomphacks clean-room effect scaffold (MIT licence).
 * The first valid callback captures instance[3] and, if non-null and
 * 4-byte aligned, reads its first three uint32_t fields immediately.
 * An aligned pointer can still be invalid: the read may fault the DSP.
 *
 * Wire format: seven little-endian bit-first 32-bit words transmitted
 * continuously as two-frequency square-wave FSK. 256 audio samples/bit:
 * 0: 44100/32 Hz; 1: 44100/16 Hz. Both stereo channels carry the same signal.
 * [sync, descriptor_ptr, base, exclusive_end, size_bytes, status, crc32].
 * CRC32 is IEEE reflected/zlib over the five words from descriptor_ptr
 * through status, serialized little-endian.
 */
#include <stdint.h>
#include "sh_params.h"

#define PROBE_SYNC       0xA71D5C3Eu
#define PROBE_WORDS      7u
#define PROBE_FRAMES     (PROBE_WORDS * 32u * 256u)
#define STATUS_OK        0u
#define STATUS_NULL      1u
#define STATUS_UNALIGNED 2u

void SH_AUDIO_FN(void **instance, void **ctx)
{
    uint32_t *s = (uint32_t *)instance[2];
    float *eff = (float *)ctx[1];
    const float *coef = (const float *)instance[1];
    float fade, level, amplitude;
    uint32_t pos;
    unsigned i;

    /* Preserve the working Stomphacks init guard: during effect-browser
     * preview and chain edits the audio callback may run before init.
     * On invalid state, produce silence and do NOT touch instance[3]. */
#ifdef SH_STATE_GUARD
    if (s[SH_STATE_GUARD_WORD] != SH_STATE_GUARD) {
        for (i = 0; i < SH_FRAMES; ++i) {
            eff[i] = 0.0f;
            eff[i + SH_CH_B_OFFSET] = 0.0f;
        }
        return;
    }
#endif
    fade = coef[SH_COEFF_BYPASS];
    level = coef[SH_PARAM_LEVEL];
    if (!(fade >= 0.0f && fade <= 1.0f) ||
        !(level >= 0.0f && level <= 1.6f)) {
        for (i = 0; i < SH_FRAMES; ++i) {
            eff[i] = 0.0f;
            eff[i + SH_CH_B_OFFSET] = 0.0f;
        }
        return;
    }

    /* Per-instance state: s[0]=captured; s[1]=sample position;
     * s[2..8]=telemetry words. state_bytes=64 protects these words;
     * the generated guard lives at s[16]. */
    if (s[0] == 0u) {
        uint32_t p = (uint32_t)(uintptr_t)instance[3];
        uint32_t status = STATUS_OK;
        uint32_t crc = 0xffffffffu;
        unsigned k, j;

        s[2] = PROBE_SYNC;
        s[3] = p;
        s[4] = 0u;
        s[5] = 0u;
        s[6] = 0u;
        if (p == 0u) {
            status = STATUS_NULL;
        } else if ((p & 3u) != 0u) {
            status = STATUS_UNALIGNED;
        } else {
            const volatile uint32_t *d =
                (const volatile uint32_t *)instance[3];
            /* Exactly three descriptor reads; NEVER dereference base. */
            s[4] = d[0];
            s[5] = d[1];
            s[6] = d[2];
        }
        s[7] = status;

        /* IEEE reflected CRC-32, little-endian bytes, matching zlib.crc32. */
        for (k = 3u; k <= 7u; ++k) {
            uint32_t v = s[k];
            for (j = 0u; j < 32u; ++j) {
                uint32_t odd = (crc ^ v) & 1u;
                crc = (crc >> 1) ^ (odd ? 0xedb88320u : 0u);
                v >>= 1;
            }
        }
        s[8] = crc ^ 0xffffffffu;
        s[1] = 0u;
        s[0] = 1u;
    }

    amplitude = 0.20f * fade * level;
    pos = s[1];
    for (i = 0; i < SH_FRAMES; ++i) {
        uint32_t bit_index = pos >> 8;
        uint32_t word = s[2u + (bit_index >> 5)];
        uint32_t bit = (word >> (bit_index & 31u)) & 1u;
        uint32_t mask = bit ? 15u : 31u;
        float sample = ((pos & mask) < ((mask + 1u) >> 1))
                       ? amplitude : -amplitude;
        /* Clamp and scrub non-finite output as in the Stomphacks example. */
        if (!(sample > -8.0f && sample < 8.0f)) sample = 0.0f;
        eff[i] = sample;
        eff[i + SH_CH_B_OFFSET] = sample;
        ++pos;
        if (pos == PROBE_FRAMES) pos = 0u;
    }
    s[1] = pos;
}
