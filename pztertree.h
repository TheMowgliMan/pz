#ifndef PZTERTREE_H_
#define PZTERTREE_H_

#include "pzbinutil.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct pzbinti pzbinti_t;
typedef struct pzbinti {
    void *v;
    uint8_t vlen;
    pzbinti_t *n_l;
    pzbinti_t *n_r;
    pzbinti_t *n_c;
} pzbinti_t;

typedef struct pzbint pzbint_t;
typedef struct pzbint {
    pzbinti_t *h;
} pzbint_t;

typedef struct terqueue tq_t;
typedef struct terqueue {
    pzbinti_t *i;
    tq_t *n;
} tq_t;

typedef struct pzbintret {
    void *d;
    uint8_t dlen;
} pzbint_ret_t;

typedef struct pzbinfindbetweenret {
    int64_t blockidx;
    size_t blocklen;
} pzbin_block_t;

pzbint_t *pztertree_New(void);
void pztertree_Add(pzbint_t *tree, void *d, uint8_t dlen);
pzbint_ret_t pztertree_Get(pzbint_t *head, uint8_t *key);
uint8_t *pztertree_InOrderFind(pzbint_t *head, void *match, size_t matchlen);

pzbin_block_t pztertree_DRefList_FindBetween(d_ref_list_t *search_from, size_t fromsz, d_ref_list_t *search_src, size_t srcsz, uint16_t min_size);

#endif
