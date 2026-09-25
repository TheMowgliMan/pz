#ifndef INSERT_INTO_H_
#define INSERT_INTO_H_

#include <stdint.h>
#include <stdlib.h>

void __insert_into(uint8_t **ptr, uint8_t ins, size_t *ptrsz, size_t *ptrsz_in_use);
void __insert_multiple_into(uint8_t **ptr, uint8_t *ins, size_t *ptrsz, size_t *ptrsz_in_use);
void __insert_multiple_into_sized(uint8_t **ptr, uint8_t *ins, size_t ins_sz, size_t *ptrsz, size_t *ptrsz_in_use);

#endif
