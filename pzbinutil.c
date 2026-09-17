#include "pzbinutil.h"

#include "macros.h"

ref_list_t *pzbinutil_FromBinary(uint8_t *bin, uint64_t bin_len) {
    ref_list_t *ret = NULL;

    uint8_t byte_buf[bin_len * 4];
    for (uint64_t i = 0; i < bin_len; i++) {
        byte_buf[i * 4] = (bin[i] & 0b11000000) >> 6;
        byte_buf[i * 4 + 1] = (bin[i] & 0b00110000) >> 4;
    }
}
