#ifndef PZCOMPRESSOR_H_
#define PZCOMPRESSOR_H_

#include "pztertree.h"

typedef struct pzcompressorinst pz_comp_inst_t;
typedef struct pzcompressorinst {
    uint8_t used_symbols;
    char **symbols;

    pzbint_t *tree;
} pz_comp_inst_t;

#endif
