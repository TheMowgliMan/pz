#ifndef PZTERTREE_H_
#define PZTERTREE_H_

#include <stdint.h>

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

pzbint_t *pztertree_New(void);
void pztertree_Add(pzbint_t *tree, void *d, uint8_t dlen);

#endif
