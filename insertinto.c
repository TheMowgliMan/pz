#include <insertinto.h>

#include "macros.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void __insert_into(uint8_t **ptr, uint8_t ins, size_t *ptrsz, size_t *ptrsz_in_use) {
    if (*ptrsz_in_use < *ptrsz) {
        (*ptr)[*ptrsz_in_use] = ins;
        (*ptrsz_in_use)++;
    } else {
        uint8_t *t = (uint8_t *)realloc(*ptr, sizeof(uint8_t) * (*ptrsz + DECOMPRESS_OVER_ALLOCATE_SIZE));
        if (t != NULL) {
            *ptr = t;
            (*ptr)[*ptrsz_in_use] = ins;
            (*ptrsz_in_use)++;
            (*ptrsz) += DECOMPRESS_OVER_ALLOCATE_SIZE;
        } else {
            debug("Error: realloc() failed!");
        }
    }
}

void __insert_multiple_into(uint8_t **ptr, uint8_t *ins, size_t *ptrsz, size_t *ptrsz_in_use) {
    for (uint32_t i = 0; i < (strlen((char *)ins) + 1); i++)
        __insert_into(ptr, ins[i], ptrsz, ptrsz_in_use);
}

void __insert_multiple_into_sized(uint8_t **ptr, uint8_t *ins, size_t ins_sz, size_t *ptrsz, size_t *ptrsz_in_use) {
    for (range_u64(i, 0, ins_sz, 1))
        __insert_into(ptr, ins[i], ptrsz, ptrsz_in_use);
}
