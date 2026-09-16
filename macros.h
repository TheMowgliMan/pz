#ifndef MACROS_H_
#define MACROS_H_

#define debug(format, ...) fprintf(stderr, format, __VA_ARGS__)

#define pzmalloc(...) malloc(__VA_ARGS__)

#endif
