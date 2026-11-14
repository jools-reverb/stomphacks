/* Desktop fault-injection harness for the exact included target phase logic */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../effects/delay-memory-write-probe/state.h"
static MemoryWriteState state;
static uint32_t memory[MW_SIZE / 4u + 2u];
static uint32_t descriptor[3], descriptor_address, protected_address[4];
static unsigned accesses, writes, max_accesses, descriptor_reads, callbacks;
static unsigned fault, hit, completed[6], test_count;
static int coeff_ok, enable, fully_on;
static FILE *capture;
static uint32_t descriptor_read(unsigned index)
{
    assert(descriptor_address && !(descriptor_address & 3u));
    ++descriptor_reads;
    return descriptor[index];
}
static unsigned word_index(uint32_t offset)
{
    assert(!(offset & 3u));
    assert(offset < 32768u || (offset >= MW_TAIL && offset < MW_SIZE));
    if (fault == 3u && offset >= MW_TAIL) {
        return 1u + (offset - MW_TAIL) / 4u;
    }
    return 1u + offset / 4u;
}
static void store_word(uint32_t offset, uint32_t value)
{
    assert(state.status == 1u && state.phase >= 2u && state.phase <= 4u);
    assert(state.stage == 0u);
    ++accesses;
    ++writes;
    memory[word_index(offset)] = value;
    if (fault == 1u && !hit) {
        memory[word_index(offset)] ^= 1u;
        hit = 1u;
    }
    if (fault == 4u && offset == MW_TAIL) {
        memory[1] ^= 1u;
    }
}
static uint32_t load_word(uint32_t offset)
{
    ++accesses;
    return memory[word_index(offset)];
}
#define MW_DESCRIPTOR(i) descriptor_read(i)
#define MW_STORE(off, value) store_word(off, value)
#define MW_LOAD(off) load_word(off)
static void tick(void)
{
    MemoryWriteState *s = &state;
    unsigned previous_phase = s->phase;
    unsigned previous_status = s->status;
    unsigned previous_stage = s->stage;
    unsigned previous_wait = s->wait_frames;
    accesses = 0u;
    writes = 0u;
    ++callbacks;
#include "../../effects/delay-memory-write-probe/step.inc"
    assert(accesses <= 16u);
    if (previous_status == 2u || previous_phase == 1u ||
        (previous_status == 1u && previous_stage == 1u)) {
        assert(accesses == 0u);
    }
    if (previous_status == 1u && previous_stage == 1u && s->status == 1u) {
        assert(s->wait_frames == (previous_wait > 16u ? previous_wait - 16u : 0u));
    }
    if (accesses > max_accesses) {
        max_accesses = accesses;
    }
    if (previous_status >= 3u) {
        assert(accesses == 0u);
    }
    if (!coeff_ok) {
        assert(accesses == 0u);
        return;
    }
#include "../../effects/delay-memory-write-probe/packet.inc"
    if (s->txpos < MW_PACKET_FRAMES) {
        s->txpos += 16u;
        if (s->txpos == MW_PACKET_FRAMES) {
            if (capture) {
                uint32_t stamp = callbacks * 16u;
                assert(fwrite(&stamp, 4, 1, capture) == 1);
                assert(fwrite(s->packet, 4, 20, capture) == 20);
            }
            if (s->packet[4] == 2u) {
                ++completed[s->packet[3]];
            }
#include "../../effects/delay-memory-write-probe/transmitted.inc"
        }
    }
    if (s->phase > previous_phase && previous_phase != 0u) {
        assert(completed[previous_phase] >= 2u);
    }
    assert(memory[0] == 0x12345678u);
    assert(memory[MW_SIZE / 4u + 1u] == 0x87654321u);
    assert(memory[8193] == 0xaaaaaaaa);
    assert(memory[MW_TAIL / 4u] == 0xaaaaaaaa);
}
static void reset(void)
{
    unsigned i;
    memset(&state, 0, sizeof(state));
    for (i = 0; i < MW_SIZE / 4u + 2u; ++i) {
        memory[i] = 0xaaaaaaaau;
    }
    memory[0] = 0x12345678u;
    memory[MW_SIZE / 4u + 1u] = 0x87654321u;
    descriptor[0] = MW_BASE;
    descriptor[1] = MW_END;
    descriptor[2] = MW_SIZE;
    descriptor_address = 0xc02758a8u;
    protected_address[0] = descriptor_address;
    protected_address[1] = 0xc0200000u;
    protected_address[2] = 0xc0201000u;
    protected_address[3] = 0xc0202000u;
    coeff_ok = 1;
    enable = 0;
    fully_on = 1;
    accesses = writes = max_accesses = descriptor_reads = callbacks = 0;
    fault = hit = 0u;
    memset(completed, 0, sizeof(completed));
    tick();
}
static void start(void)
{
    enable = 1;
    tick();
}
static void reach(unsigned phase, unsigned stage)
{
    unsigned limit = callbacks + 300000u;
    while (!(state.phase == phase && state.stage == stage && state.status == 1u)) {
        assert(state.status < 3u);
        assert(callbacks < limit);
        tick();
    }
}
static void terminal(void)
{
    while (state.status < 3u && callbacks < 300000u) {
        tick();
    }
    assert(state.status >= 3u);
}
static void failed(unsigned error)
{
    MemoryWriteState frozen;
    unsigned i, reads;
    terminal();
    assert((state.detail & 255u) == error);
    assert(state.status == (error == 7u ? 5u : 4u));
    frozen = state;
    reads = descriptor_reads;
    for (i = 0; i < 21000u; ++i) {
        enable = i & 1u;
        tick();
        assert(!accesses);
    }
    assert(descriptor_reads == reads);
    assert(state.fail_offset == frozen.fail_offset);
    assert(state.expected == frozen.expected && state.observed == frozen.observed);
    assert(state.detail == frozen.detail && state.phase == frozen.phase);
    assert(state.packet[4] == state.status && state.packet[6] == state.detail);
    ++test_count;
}
int main(int argc, char **argv)
{
    unsigned i, j;
    assert(sizeof(state) == 184u);
    /* Full run, real packet duration, worst residual idle packet */
    reset();
    if (argc == 2) {
        capture = fopen(argv[1], "wb");
        assert(capture);
    }
    start();
    terminal();
    assert(state.status == 3u && state.mask == 31u && state.highwater == 32768u);
    assert(state.elapsed < 100u * 44100u);
    printf("PASS full run: %.6f seconds to terminal state, state=%zu bytes, max accesses=%u\n",
           state.elapsed / 44100.0, sizeof(state), max_accesses);
    for (i = 0; i < 32768u / 4u; ++i) {
        assert(memory[i + 1u] == MW_PATTERN(i, 0, 1));
    }
    for (i = 0; i < 4u; ++i) {
        assert(memory[MW_TAIL / 4u + 1u + i] == MW_PATTERN(i, 1, 1));
    }
    for (i = 0; i < 40000u; ++i) {
        enable = i & 1;
        tick();
        assert(!accesses);
    }
    if (capture) {
        fclose(capture);
        capture = NULL;
    }
    ++test_count;
    reset();
    descriptor_address += 24u;
    protected_address[0] = descriptor_address;
    start();
    terminal();
    assert(state.status == 3u);
    ++test_count;
    for (i = 0; i < 2u; ++i) {
        reset();
        descriptor_address = i ? 0xc02758a9u : 0u;
        protected_address[0] = descriptor_address;
        start();
        assert(!descriptor_reads);
        failed(1u);
    }
    /* Reversed, undersized, overflowing and individually changed fields */
    reset();
    descriptor_address = 0xfffffffcu;
    protected_address[0] = descriptor_address;
    start();
    assert(!descriptor_reads);
    failed(3u);
    for (i = 0; i < 6u; ++i) {
        reset();
        if (i < 3u) {
            descriptor[i] += 4u;
        } else if (i == 3u) {
            descriptor[1] = MW_BASE;
        } else if (i == 4u) {
            descriptor[0] = 0xfffffff8u;
            descriptor[1] = 8u;
            descriptor[2] = 16u;
        } else {
            descriptor[1] = MW_BASE + 16u;
            descriptor[2] = 16u;
        }
        start();
        assert(!accesses);
        failed(2u);
    }
    for (i = 0; i < 4u; ++i) {
        for (j = 0; j < 2u; ++j) {
            reset();
            protected_address[i] = j ? MW_END - 4u : MW_BASE;
            start();
            failed(3u);
        }
    }
    reset();
    protected_address[1] = 0xfffffffcu;
    start();
    failed(3u);
    for (i = 0; i < 4u; ++i) {
        reset();
        start();
        reach(2u, 0u);
        if (!i) {
            descriptor_address += 12u;
        } else {
            descriptor[i - 1u] += 4u;
        }
        tick();
        assert(!accesses);
        failed(4u);
    }
    for (i = 1u; i <= 4u; ++i) {
        reset();
        fault = i;
        start();
        if (i == 2u) {
            reach(2u, 1u);
            memory[1] ^= 2u;
        }
        terminal();
        assert(state.expected != state.observed);
        failed(5u);
    }
    /* Corruption at the very last word, and final pre/post-head tail checks */
    for (i = 0; i < 4u; ++i) {
        reset();
        start();
        reach(5u, i == 3u ? 6u : 1u);
        memory[(i == 0u ? 32764u : (i == 1u ? MW_TAIL : MW_SIZE - 4u)) / 4u + 1u] ^= 1u;
        failed(5u);
    }
    reset();
    start();
    reach(2u, 0u);
    state.cursor = 3u;
    failed(6u);
    reset();
    start();
    reach(5u, 1u);
    state.stage = 0u;
    failed(6u);
    /* Exactly adjacent protected extents do not overlap either interval */
    reset();
    protected_address[1] = MW_BASE - 192u;
    protected_address[2] = MW_BASE + 32768u;
    protected_address[3] = MW_END;
    start();
    terminal();
    assert(state.status == 3u);
    ++test_count;
    reset();
    start();
    assert(state.status == 2u);
    enable = 0;
    failed(7u);
    reset();
    start();
    reach(5u, 1u);
    fully_on = 0;
    failed(7u);
    /* Abort on Enable off, bypass, or invalid-coefficient resumption */
    for (i = 0; i < 3u; ++i) {
        reset();
        start();
        reach(3u, 0u);
        if (!i) {
            enable = 0;
        } else if (i == 1u) {
            fully_on = 0;
        } else {
            coeff_ok = 0;
            tick();
            assert(!accesses);
            coeff_ok = 1;
        }
        failed(7u);
    }
    /* Initial Enable=1, bypassed transition, invalid init and reinit */
    reset();
    memset(&state, 0, sizeof(state));
    enable = 1;
    for (i = 0; i < 100u; ++i) {
        tick();
        assert(state.status == 0u && !accesses && !descriptor_reads);
    }
    enable = 0;
    tick();
    fully_on = 0;
    enable = 1;
    tick();
    fully_on = 1;
    tick();
    assert(state.status == 0u);
    enable = 0;
    tick();
    start();
    assert(state.phase == 1u);
    state.magic = 0u;
    tick();
    assert(state.status == 0u);
    coeff_ok = 0;
    enable = 0;
    tick();
    coeff_ok = 1;
    enable = 1;
    tick();
    assert(state.status == 0u);
    ++test_count;
    printf("PASS %u engine scenarios; packet gates, first-failure freeze and canaries checked\n", test_count);
    return 0;
}
