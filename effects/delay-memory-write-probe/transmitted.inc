/* Called by textual inclusion only, after the last sample of a packet */
++s->sequence;
if (s->status == 2u && s->packet[4] == 2u &&
    s->packet[3] == s->phase) {
    ++s->gate;
    if (s->gate == 2u) {
        ++s->phase;
        s->status = 1u;
        s->gate = 0u;
        s->cursor = 0u;
        s->stage = 0u;
        s->detail = 0u;
        s->offset = 0u;
        s->length = s->phase == 3u ? 256u : 16u;
        if (s->phase >= 4u) {
            s->offset = MW_TAIL;
        }
        if (s->phase == 5u) {
            s->detail = 0x00010200u;
            s->stage = 1u;
            s->wait_frames = 441000u;
        }
    }
}
