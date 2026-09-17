#ifndef PZBINUTIL_H_
#define PZBINUTIL_H_

#include <stdint.h>

typedef struct refarr ref_list_t;
typedef struct refarr {
    ref_list_t *n;
    uint8_t *d;
} ref_list_t;

#endif
