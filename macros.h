#ifndef MACROS_H_
#define MACROS_H_

#include <stdio.h>
#include <stdlib.h>

#define debug(format, ...) fprintf(stderr, format __VA_OPT__(,) __VA_ARGS__)

#define pzmalloc(...) malloc(__VA_ARGS__)
#define pzfree(...) free(__VA_ARGS__)

#endif
