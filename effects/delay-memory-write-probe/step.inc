/* Included directly in the target kernel and host harness: no call boundary.
 * Inputs: s, descriptor_address, protected_address[4], coeff_ok, enable,
 * fully_on. Access macros are volatile words on target and checked on host.
 * This block performs at most sixteen sample-memory accesses per callback.
 */
{
    unsigned k;
    uint32_t error = 0u;
    const uint32_t protected_length[4] = {12u, 192u, 64u, 128u};
    if (s->magic != MW_MAGIC) {
        uint32_t *words = (uint32_t *)s;
        for (k = 0; k < sizeof(*s) / 4u; ++k) {
            words[k] = 0u;
        }
        s->magic = MW_MAGIC;
        s->fail_offset = 0xffffffffu;
        s->txpos = MW_PACKET_FRAMES;
    }
    if (!coeff_ok) {
        if (s->status == 1u || s->status == 2u) {
            s->invalid = 1u;
        }
        s->seen_zero = 0u;
    } else {
        if (s->invalid && (s->status == 1u || s->status == 2u)) {
            MW_FAIL(7u);
        }
        if (s->status == 0u) {
            if (!enable) {
                s->seen_zero = 1u;
            } else {
                if (s->seen_zero && fully_on) {
                    s->status = 1u;
                    s->phase = 1u;
                    s->descriptor = descriptor_address;
                }
                /* A transition while bypassed cannot start on a later fade */
                s->seen_zero = 0u;
            }
        }
        if (s->status == 1u || s->status == 2u) {
            s->elapsed += 16u;
            if (!enable || !fully_on) {
                MW_FAIL(7u);
            }
        }
        if (s->status == 1u) {
            if (descriptor_address != s->descriptor) {
                error = 4u;
            } else if (!descriptor_address || (descriptor_address & 3u)) {
                error = 1u;
            } else if (descriptor_address > 0xffffffffu - 11u) {
                error = 3u;
            } else {
                uint32_t b = MW_DESCRIPTOR(0);
                uint32_t e = MW_DESCRIPTOR(1);
                uint32_t n = MW_DESCRIPTOR(2);
                if (s->phase == 1u) {
                    s->base = b;
                    s->end = e;
                    s->size = n;
                } else if (b != s->base || e != s->end || n != s->size) {
                    error = 4u;
                }
                if (!error && (e <= b || e - b != n || (b & 7u) ||
                    (e & 3u) || (n & 3u) || b != MW_BASE ||
                    e != MW_END || n != MW_SIZE)) {
                    error = 2u;
                }
            }
            if (!error) {
                /* Check both proposed regions against all known extents */
                if (32768u > s->size || MW_TAIL > s->size ||
                    16u > s->size - MW_TAIL || MW_TAIL < 32768u) {
                    error = 3u;
                }
                for (k = 0; k < 4u; ++k) {
                    uint32_t p = protected_address[k];
                    uint32_t n = protected_length[k];
                    if (p > 0xffffffffu - (n - 1u) ||
                        MW_OVERLAP(s->base, 32768u, p, n) ||
                        MW_OVERLAP(s->end - 16u, 16u, p, n)) {
                        error = 3u;
                    }
                }
            }
            if (error) {
                MW_FAIL(error);
            } else if (s->phase == 1u) {
                s->mask = 1u;
                s->status = 2u;
                s->gate = 0u;
            } else {
                uint32_t pattern = (s->detail >> 16) & 1u;
                uint32_t operation = s->stage == 0u ? 1u :
                                     (s->stage == 1u ? 2u : 3u);
                s->detail = (pattern << 16) | (operation << 8);
                if (s->phase < 2u || s->phase > 5u || s->stage > 6u ||
                    (s->phase < 4u && (s->stage > 2u || s->offset != 0u)) ||
                    (s->phase == 2u && s->length != 16u) ||
                    (s->phase == 3u && s->length == 16u) ||
                    (s->phase == 4u && (s->stage > 3u ||
                        (s->stage == 3u ? (s->offset != 0u || s->length != 32768u) :
                         (s->offset != MW_TAIL || s->length != 16u)))) ||
                    (s->phase == 5u && (s->stage == 0u || s->stage == 2u || s->stage == 3u ||
                        (s->stage == 5u ? (s->offset != 0u || s->length != 32768u) :
                         (s->offset != MW_TAIL || s->length != 16u)))) ||
                    (s->offset != 0u && s->offset != MW_TAIL) ||
                    (s->length != 16u && s->length != 256u &&
                     s->length != 4096u && s->length != 32768u) ||
                    (s->offset == MW_TAIL && s->length != 16u) ||
                    s->cursor > s->length || (s->cursor & 3u)) {
                    MW_FAIL(6u);
                } else if (s->offset > s->size || s->length > s->size - s->offset ||
                           (s->offset & 3u) || (s->length & 3u)) {
                    MW_FAIL(3u);
                } else if (s->stage == 1u) {
                    if (s->wait_frames > 16u) {
                        s->wait_frames -= 16u;
                    } else {
                        s->wait_frames = 0u;
                        s->cursor = 0u;
                        s->stage = s->phase == 5u ? 4u : 2u;
                    }
                } else {
                    unsigned budget = s->stage == 0u ? 8u : 16u;
                    for (k = 0; k < budget && s->cursor < s->length; ++k) {
                        uint32_t off = s->offset + s->cursor;
                        uint32_t expected = MW_PATTERN(s->cursor >> 2,
                            s->offset == MW_TAIL, s->stage == 3u ? 1u : pattern);
                        uint32_t observed;
                        if (s->stage == 0u) {
                            MW_STORE(off, expected);
                        }
                        observed = MW_LOAD(off);
                        if (observed != expected) {
                            s->fail_offset = off;
                            s->expected = expected;
                            s->observed = observed;
                            MW_FAIL(5u);
                            break;
                        }
                        s->cursor += 4u;
                    }
                    if (s->status == 1u && s->cursor == s->length) {
                        if (s->stage == 0u) {
                            s->stage = 1u;
                            s->wait_frames = 44100u;
                        } else if (s->phase == 5u) {
                            if (s->stage == 4u) {
                                s->stage = 5u;
                                s->offset = 0u;
                                s->length = 32768u;
                                s->cursor = 0u;
                            } else if (s->stage == 5u) {
                                s->stage = 6u;
                                s->offset = MW_TAIL;
                                s->length = 16u;
                                s->cursor = 0u;
                            } else {
                                s->mask |= 16u;
                                s->status = 3u;
                            }
                        } else if (s->phase == 4u && s->stage == 2u) {
                            s->stage = 3u;
                            s->offset = 0u;
                            s->length = 32768u;
                            s->cursor = 0u;
                        } else if (!pattern) {
                            s->detail = 0x00010000u;
                            s->stage = 0u;
                            s->cursor = 0u;
                            if (s->phase == 4u) {
                                s->offset = MW_TAIL;
                                s->length = 16u;
                            }
                        } else {
                            if (s->phase < 4u) {
                                s->highwater = s->length;
                            }
                            if (s->phase == 3u && s->length < 32768u) {
                                s->length = s->length == 256u ? 4096u : 32768u;
                                s->detail = 0u;
                                s->stage = 0u;
                                s->cursor = 0u;
                            } else {
                                s->mask |= 1u << (s->phase - 1u);
                                s->status = 2u;
                                s->gate = 0u;
                            }
                        }
                    }
                }
            }
        }
    }
}
