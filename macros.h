#ifndef MACROS_H_
#define MACROS_H_

#include "pzbinutil.h"
#include <stdio.h>
#include <stdlib.h>

#define debug(format, ...) fprintf(stderr, format __VA_OPT__(,) __VA_ARGS__)

#define STRINGIFY(x) #x
#define STREXPR(x) STRINGIFY(x)

#define CATTOK(x, y) x ## y

#define assertif(test) (test && debug("Error: file " __FILE__ ": line " STREXPR(__LINE__)": assertion failed: '" STREXPR(test) "'!\n"))

#define ITERATOR CATTOK(iter_, __LINE__)

#define range_u64(id, start, stop, step) uint64_t id = (start); id != (stop); id += (step)
#define range_u32(id, start, stop, step) uint32_t id = (start); id != (stop); id += (step)
#define range_u16(id, start, stop, step) uint16_t id = (start); id != (stop); id += (step)
#define range_u8(id, start, stop, step) uint8_t id = (start); id != (stop); id += (step)
#define range_d_ref_list(id, x) d_ref_list_t *id = x; id != NULL; id = id->n

#define pzmalloc(...) malloc(__VA_ARGS__)
#define pzfree(...) free(__VA_ARGS__)

#ifndef DECOMPRESS_OVER_ALLOCATE_SIZE
#define DECOMPRESS_OVER_ALLOCATE_SIZE 256
#endif

#ifndef MAXIMUM_SYMBOL_COUNT
#define MAXIMUM_SYMBOL_COUNT 16
#endif

#endif
