#ifndef PZTERTREE_H_
#define PZTERTREE_H_

#include <stdint.h>

typedef struct pzbinti pzbinti_t;
typedef struct pzbinti {
    uint8_t v;
    pzbinti_t *n;
} pzbinti_t;

typedef struct pzbint pzbint_t;
typedef struct pzbint {
    pzbinti_t *h;
} pzbint_t;

pzbint_t *pztertree_New(void);

#endif
