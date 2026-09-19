#include "pzbinutil.h"

#include "macros.h"

ref_list_t *pzbinutil_FromBinary(uint8_t *bin, uint64_t bin_len) {
    ref_list_t *ret = NULL;

    uint8_t *byte_buf = (uint8_t *)pzmalloc(sizeof(uint8_t) * bin_len * 4); // DON'T FREE THIS, IT'S USED AS THE DATAREF FOR THE PART BELOW
    for (uint64_t i = 0; i < bin_len; i++) {
        byte_buf[i * 4] = (bin[i] & 0b11000000) >> 6;
        byte_buf[i * 4 + 1] = (bin[i] & 0b00110000) >> 4;
        byte_buf[i * 4 + 2] = (bin[i] & 0b00001100) >> 2;
        byte_buf[i * 4 + 3] = bin[i] & 0b00000011;
    }

    ref_list_t *head = (ref_list_t *)pzmalloc(sizeof(ref_list_t));
    ret = head;

    head->n = NULL;

    uint64_t last_idx = 0;
    for (uint64_t i = 0; i < (bin_len * 4); i++) {
        if (byte_buf[i] == 0) {
            head->d = &byte_buf[last_idx];
            head->n = (ref_list_t *)pzmalloc(sizeof(ref_list_t));

            head = head->n;
            head->n = NULL;

            last_idx = i + 1;
        }
    }

    return ret;
}

void pzbinutil_DRefList_DelLeft(d_ref_list_t *ptr) {
    d_ref_list_t *old = ptr;
    ptr = old->n;
    ptr->p = NULL;
    free(old);
}
