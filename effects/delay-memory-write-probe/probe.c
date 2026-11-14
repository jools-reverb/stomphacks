/* Bounded memory probe. Experimental: not hardware verified.
 * The shared blocks are included into this leaf kernel without calls.
 * Per-instance state holds one immutable packet and live first-failure data.
 */
#include <stdint.h>
#include "sh_params.h"
#include "state.h"
#define MW_DESCRIPTOR(i) (((const volatile uint32_t *)instance[3])[(i)])
#define MW_STORE(off, value) \
    (*(volatile uint32_t *)(uintptr_t)(s->base + (off)) = (value))
#define MW_LOAD(off) (*(const volatile uint32_t *)(uintptr_t)(s->base + (off)))

void SH_AUDIO_FN(void **instance, void **ctx)
{
    MemoryWriteState *s = (MemoryWriteState *)instance[2];
    float *eff = (float *)ctx[1];
    const float *coef = (const float *)instance[1];
    float fade, level, enabled;
    uint32_t descriptor_address = (uint32_t)(uintptr_t)instance[3];
    uint32_t protected_address[4];
    unsigned i;
    int coeff_ok, enable, fully_on;
    if (((uint32_t *)s)[SH_STATE_GUARD_WORD] != SH_STATE_GUARD) {
        /* Force fresh disabled state if a valid guard later reappears */
        s->magic = 0u;
        for (i = 0; i < SH_FRAMES; ++i) {
            eff[i] = 0.0f;
            eff[i + SH_CH_B_OFFSET] = 0.0f;
        }
        return;
    }
    fade = coef[SH_COEFF_BYPASS];
    level = coef[SH_PARAM_LEVEL];
    enabled = coef[SH_PARAM_ENABLE];
    coeff_ok = fade >= 0.0f && fade <= 1.0f &&
               level >= 0.0f && level <= 1.6f &&
               (enabled == 0.0f || enabled == 1.0f);
    enable = enabled == 1.0f;
    fully_on = fade == 1.0f;
    protected_address[0] = descriptor_address;
    protected_address[1] = (uint32_t)(uintptr_t)s;
    protected_address[2] = (uint32_t)(uintptr_t)coef;
    protected_address[3] = (uint32_t)(uintptr_t)eff;
#include "step.inc"
    if (!coeff_ok) {
        for (i = 0; i < SH_FRAMES; ++i) {
            eff[i] = 0.0f;
            eff[i + SH_CH_B_OFFSET] = 0.0f;
        }
        return;
    }
#include "packet.inc"
    for (i = 0; i < SH_FRAMES; ++i) {
        float sample = 0.0f;
        if (s->txpos < MW_PACKET_FRAMES) {
            uint32_t bit_index = s->txpos >> 8;
            uint32_t bit = (s->packet[bit_index >> 5] >> (bit_index & 31u)) & 1u;
            uint32_t mask = bit ? 15u : 31u;
            float amplitude = 0.20f * fade * level;
            sample = ((s->txpos & mask) < ((mask + 1u) >> 1)) ? amplitude : -amplitude;
            ++s->txpos;
            if (s->txpos == MW_PACKET_FRAMES) {
#include "transmitted.inc"
            }
        }
        if (!(sample > -8.0f && sample < 8.0f)) {
            sample = 0.0f;
        }
        eff[i] = sample;
        eff[i + SH_CH_B_OFFSET] = sample;
    }
}
