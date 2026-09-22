#ifndef MACROS_H_
#define MACROS_H_

#include <stdio.h>
#include <stdlib.h>

#define debug(format, ...) fprintf(stderr, format __VA_OPT__(,) __VA_ARGS__)

#define STRINGIFY(x) #x
#define STREXPR(x) STRINGIFY(x)

#define CATTOK(x, y) x ## y

#define assertif(test) (test && debug("Error: file " __FILE__ ": line " STREXPR(__LINE__)": assertion failed: '" STREXPR(test) "'!\n"))

#define ITERATOR CATTOK(iter_, __LINE__)
#define range_int(x) uint64_t ITERATOR = 0; ITERATOR < x; ITERATOR++

#define pzmalloc(...) malloc(__VA_ARGS__)
#define pzfree(...) free(__VA_ARGS__)

#ifndef DECOMPRESS_OVER_ALLOCATE_SIZE
#define DECOMPRESS_OVER_ALLOCATE_SIZE 256
#endif

#ifndef MAXIMUM_SYMBOL_COUNT
#define MAXIMUM_SYMBOL_COUNT 16
#endif

#endif
