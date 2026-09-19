#include "pzcompressor.h"

#include "pzbinutil.h"
#include "pztertree.h"
#include "macros.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void __insert_into(uint8_t *ptr, uint8_t ins, size_t *ptrsz, size_t *ptrsz_in_use) {
    if (*ptrsz_in_use < *ptrsz) {
        ptr[*ptrsz_in_use] = ins;
        ptrsz_in_use++;
    } else {
        uint8_t *t = (uint8_t *)realloc(ptr, sizeof(uint8_t) * (*ptrsz + DECOMPRESS_OVER_ALLOCATE_SIZE));
        if (t) {
            ptr = t;

            ptr[*ptrsz_in_use] = ins;
            ptrsz_in_use++;
        } else {
            debug("Error: realloc() failed!");
        }
    }
}

void pzcompressor_ImportTreeFile(pz_comp_inst_t *inst, char *tree_string, uint32_t tree_size) {
    inst->tree = pztertree_New();
    inst->symbols = (char**)pzmalloc(sizeof(char *) * 16);
    inst->used_symbols = 0;

    char *string = (char *)pzmalloc(sizeof(char) * (tree_size + 1));
    memcpy(string, tree_string, tree_size);
    string[tree_size] = '\0';

    for (char *ln = strtok(string, "\n"); ln != NULL; strtok(NULL, "\n")) {
        printf("%s\n", ln);

        char f = ln[0];
        if (f >= '0' && f <= '9') {
            int d = atoi(ln);

            char *dc = (char *)pzmalloc(sizeof(char));
            *dc = d & 0xff;

            pztertree_Add(inst->tree, &d, sizeof(int));
        } else {
            pztertree_Add(inst->tree, ln, sizeof(char) * (1 + strlen(ln)));
        }

        if (f == '[') {
            inst->symbols[inst->used_symbols] = (char *)pzmalloc(sizeof(char) * (strlen(ln) + 1));
            memcpy(inst->symbols[inst->used_symbols], ln, sizeof(char) * (1 + strlen(ln)));
            inst->used_symbols++;
        }
    }

    pzfree(string);
}

uint8_t *pzcompressor_DecompressFile(pz_comp_inst_t *inst, uint8_t *fdata, size_t fdatalen) {
    uint32_t tree_size = 0;

    tree_size |= fdata[3];
    tree_size |= fdata[2] << 8;
    tree_size |= fdata[1] << 16;
    tree_size |= fdata[0] << 24;

    tree_size += 4;
    char *tree_string = (char *)(&fdata[4]);

    /* FIXME: previous tree and symbols will be memory leaked */
    pzcompressor_ImportTreeFile(inst, tree_string, tree_size);

    uint8_t *bin = &fdata[tree_size + 4];
    ref_list_t *data = pzbinutil_FromBinary(bin, fdatalen - (tree_size + 4));

    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * DECOMPRESS_OVER_ALLOCATE_SIZE);
    size_t retsz = DECOMPRESS_OVER_ALLOCATE_SIZE;
    size_t retsz_in_use = 0;

    bool is_meta = false;
    int pz_protocol_ver = -1;

    uint8_t reptstage = 0;
    size_t reptcount = 0;
    uint8_t *reptcc = NULL;
    uint16_t reptcc_i = 0;

    d_ref_list_t *bref_q_head = (d_ref_list_t *)pzmalloc(sizeof(d_ref_list_t));
    bref_q_head->n = NULL;
    bref_q_head->p = NULL;
    d_ref_list_t *bref_q_tail = bref_q_head;
    size_t bref_q_sz = 0;

    uint8_t bref_stage = 0;
    uint8_t *bref_cc = NULL;

    size_t bref_len = 0;
    size_t bref_count = 0;

    uint64_t bref_char_passed = 0;

    for (ref_list_t *cc = data; cc != NULL; cc = cc->n) {
        while (bref_q_sz > 65536) {
            pzbinutil_DRefList_DelLeft(bref_q_head);
            bref_q_sz--;
        }

        if (reptstage > 0) {
            if (reptstage == 1) {
                if (reptcc == NULL) {
                    reptcc_i = 0;
                    reptcc = (uint8_t *)pzmalloc(sizeof(uint8_t) * 512);
                    memset(reptcc, 0, sizeof(uint8_t) * 512);
                }

                uint32_t cc_sz = strlen((char *)(cc->d)) + 1;
                memcpy(&reptcc[reptcc_i], cc->d, cc_sz);
                reptcc_i += cc_sz;

                if (reptcc_i == 5) {
                    reptstage += 1;
                    reptcount = reptcc[0] * 64 + reptcc[1] * 16 + reptcc[2] * 4 + reptcc[3];

                    free(reptcc);
                    reptcc = NULL;
                }
            } else if (reptstage == 2) {
                pzbint_ret_t c = pztertree_Get(inst->tree, cc->d);

                if (c.dlen > 1) {
                    debug("Error: Bad character: too long, not a character!");
                }

                for (uint64_t j = 0; j < reptcount; j++) {
                    __insert_into(ret, ((uint8_t *)(c.d))[0], &retsz, &retsz_in_use); // This modifies the variables in-place for me
                    pzbinutil_DRefList_DelLeft(bref_q_head);

                    d_ref_list_t *new = (d_ref_list_t *)pzmalloc(sizeof(d_ref_list_t));
                    new->d = (uint8_t *)(c.d);
                    new->n = NULL;
                    new->p = bref_q_tail;

                    bref_q_tail->n = new;
                    bref_q_tail = new;

                    bref_char_passed++;
                }

                /* Don't need to free reptcc here as we do that above */
                reptstage = 0;
                reptcount = 0;
                reptcc_i = 0;
            }
        }
    }
}
