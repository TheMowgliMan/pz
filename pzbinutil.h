#ifndef PZBINUTIL_H_
#define PZBINUTIL_H_

#include <stdint.h>

typedef struct refarr ref_list_t;
typedef struct refarr {
    ref_list_t *n;
    uint8_t *d;
} ref_list_t;

ref_list_t *pzbinutil_FromBinary(uint8_t *bin, uint64_t bin_len);

#endif
