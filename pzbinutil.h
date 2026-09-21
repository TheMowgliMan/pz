#ifndef PZBINUTIL_H_
#define PZBINUTIL_H_

#include <stdint.h>

typedef struct refarr ref_list_t;
typedef struct refarr {
    ref_list_t *n;
    uint8_t *d;
} ref_list_t;

typedef struct drefarr d_ref_list_t;
typedef struct drefarr {
    d_ref_list_t *n;
    d_ref_list_t *p;
    uint8_t *d;
} d_ref_list_t;

ref_list_t *pzbinutil_FromBinary(uint8_t *bin, uint64_t bin_len);

void pzbinutil_DRefList_DelLeft(d_ref_list_t **ptr);
void pzbinutil_DRefList_Append(d_ref_list_t **ptr, uint8_t *d);
d_ref_list_t *pzbinutil_DRefList_ReelFromRight(d_ref_list_t *ptr, uint32_t reel);

#endif
