/* One CRC word per callback, at most 32 CRC bit steps; no second packet */
if (s->txpos == MW_PACKET_FRAMES) {
    if (s->crc_word == 0u) {
        s->packet[0] = MW_SYNC;
        s->packet[1] = 1u;
        s->packet[2] = s->sequence;
        s->packet[3] = s->phase;
        s->packet[4] = s->status;
        s->packet[5] = s->mask;
        s->packet[6] = s->detail;
        s->packet[7] = s->elapsed;
        s->packet[8] = s->descriptor;
        s->packet[9] = s->base;
        s->packet[10] = s->end;
        s->packet[11] = s->size;
        s->packet[12] = s->offset;
        s->packet[13] = s->length;
        s->packet[14] = s->cursor;
        s->packet[15] = s->highwater;
        s->packet[16] = s->fail_offset;
        s->packet[17] = s->expected;
        s->packet[18] = s->observed;
        s->crc = 0xffffffffu;
        s->crc_word = 1u;
    }
    {
        uint32_t v = s->packet[s->crc_word];
        unsigned bit;
        for (bit = 0; bit < 32u; ++bit) {
            uint32_t odd = (s->crc ^ v) & 1u;
            s->crc = (s->crc >> 1) ^ (odd ? 0xedb88320u : 0u);
            v >>= 1;
        }
    }
    ++s->crc_word;
    if (s->crc_word == 19u) {
        s->packet[19] = s->crc ^ 0xffffffffu;
        s->crc_word = 0u;
        s->txpos = 0u;
    }
}
