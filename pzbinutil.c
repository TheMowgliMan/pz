#include "pzbinutil.h"

#include "macros.h"
#include "insertinto.h"

#include <string.h>
#include <stdlib.h>

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
            uint8_t *temp = &byte_buf[last_idx];
            head->d = (uint8_t *)pzmalloc(sizeof(uint8_t *) * (strlen((char *)temp) + 1));
            memcpy(head->d, temp, (strlen((char *)temp) + 1));
            head->n = (ref_list_t *)pzmalloc(sizeof(ref_list_t));

            head = head->n;
            head->n = NULL;

            last_idx = i + 1;
        }
    }

    return ret;
}

uint8_t *pzbinutil_IntArr_ToBinary(uint8_t *data, size_t data_sz, size_t *datasz_ret) {
    uint8_t byte_buf[4] = {0, 0, 0, 0};

    *datasz_ret = 0;

    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * DECOMPRESS_OVER_ALLOCATE_SIZE);
    size_t retsz = DECOMPRESS_OVER_ALLOCATE_SIZE;
    size_t retsz_in_use = 0;

    uint8_t last_id = 0;
    for (range_u64(jj, 0, data_sz, 1)) {
        byte_buf[jj] = data[jj];

        if ((jj % 4) == 3) {
            uint8_t a = byte_buf[3];
            a |= byte_buf[2] << 2;
            a |= byte_buf[1] << 4;
            a |= byte_buf[0] << 6;

            __insert_into(&ret, a, &retsz, &retsz_in_use);
            (*datasz_ret)++;
        }

        last_id = (jj % 4);
    }

    if (last_id != 3) {
        while (last_id != 3) {
            byte_buf[last_id] = 0;
            last_id++;
        }

        uint8_t a = byte_buf[3];
        a |= byte_buf[2] << 2;
        a |= byte_buf[1] << 4;
        a |= byte_buf[0] << 6;

        __insert_into(&ret, a, &retsz, &retsz_in_use);
        (*datasz_ret)++;
    }

    return ret;
}

void pzbinutil_DRefList_DelLeft(d_ref_list_t **ptr) {
    d_ref_list_t *old = *ptr;
    *ptr = old->n;
    if (!((*ptr) == NULL)) (*ptr)->p = NULL;
    free(old);
}

void pzbinutil_DRefList_Append(d_ref_list_t **ptr, uint8_t *d) {
    if (!((*ptr)->d)) {
        (*ptr)->d = d;
        return;
    }

    d_ref_list_t *new = (d_ref_list_t *)pzmalloc(sizeof(d_ref_list_t));
    new->d = d;
    new->n = NULL;
    new->p = *ptr;

    (*ptr)->n = new;
    *ptr = new;
}

d_ref_list_t *pzbinutil_DRefList_ReelFromRight(d_ref_list_t *ptr, uint32_t reel) {
    d_ref_list_t *tail = ptr;
    for (uint32_t i = 0; i < reel; i++) {
        tail = tail->p;

        if (tail == NULL) {
            debug("Error: reeled past the end of d_ref_list_t!");
            goto end;
        }
    }

end:
    return tail;
}

uint8_t *pzbinutil_KaboomChar(uint8_t cc) {
    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * 5);
    ret[4] = 0;

    ret[0] = (cc & 0b11000000) >> 6;
    ret[1] = (cc & 0b00110000) >> 4;
    ret[2] = (cc & 0b00001100) >> 2;
    ret[3] = cc & 0b00000011;

    return ret;
}

uint8_t *pzbinutil_KaboomShort(uint16_t cc) {
    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * 9);
    ret[8] = 0;

    for (range_u8(i, 0, 8, 1)) {
        ret[i] = (cc & (0x03 << ((7 - i) * 2))) >> ((7 - i) * 2);
    }

    return ret;
}

uint8_t *pzbinutil_KaboomDWord(uint32_t cc) {
    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * 17);
    ret[16] = 0;

    for (range_u8(i, 0, 16, 1)) {
        ret[i] = (cc & (0x03 << ((15 - i) * 2))) >> ((15 - i) * 2);
    }

    return ret;
}
