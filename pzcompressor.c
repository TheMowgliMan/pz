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

bool match_sym(pz_comp_inst_t *inst, char *sym_name, uint8_t *sym_key) {
    uint8_t *key = NULL;
    for (uint16_t i = 0; i < MAXIMUM_SYMBOL_COUNT; i++) {
        if ((size_t)(inst->symbols[i]) == 0) // Evil pointer casting lol
            continue;

        if (strcmp(inst->symbols[i], sym_name) == 0) {
            key = inst->symbol_references[i];
        }
    }

    if (!key) {
        return false;
    } else {
        int test = strcmp((char *)sym_key, (char *)key);

        switch (test) {
            case 0:
                return true;
                break;
            default:
                return false;
        }
    }
}

void pzcompressor_ImportTreeFile(pz_comp_inst_t *inst, char *tree_string, uint32_t tree_size) {
    inst->tree = pztertree_New();
    inst->symbols = (char**)pzmalloc(sizeof(char *) * MAXIMUM_SYMBOL_COUNT);
    memset(inst->symbols, 0, sizeof(char *) * MAXIMUM_SYMBOL_COUNT);
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

            uint8_t *sym_key = pztertree_InOrderFind(inst->tree, ln, strlen(ln) + 1);
            inst->symbol_references[inst->used_symbols] = sym_key;

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
    uint16_t bref_cc_i = 0;

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

                    pzbinutil_DRefList_Append(bref_q_tail, (uint8_t *)(c.d));

                    bref_char_passed++;
                }

                /* Don't need to free reptcc here as we do that above */
                reptstage = 0;
                reptcount = 0;
                reptcc_i = 0;
            } else {
                debug("Error: invalid [REPT] stage: %d", reptstage);
            }

            continue;
        } else if (bref_stage > 0) {
            if (bref_stage == 1) {
                if (bref_cc == NULL) {
                    bref_cc_i = 0;
                    bref_cc = (uint8_t *)pzmalloc(sizeof(uint8_t) * 1024);
                    memset(bref_cc, 0, sizeof(uint8_t) * 1024);
                }

                uint32_t cc_sz = strlen((char *)(cc->d)) + 1;
                memcpy(&bref_cc[bref_cc_i], cc->d, cc_sz);
                bref_cc_i += cc_sz;

                if (bref_cc_i == 9) {
                    bref_stage++;
                    bref_count = bref_cc[0] * 16384 + bref_cc[1] * 4096 + bref_cc[2] * 1024 + bref_cc[3] * 256 + bref_cc[4] * 64 + bref_cc[5] * 16 + bref_cc[6] * 4 + bref_cc[7];

                    free(bref_cc);
                    bref_cc = NULL;
                }
            } else if (bref_stage == 2) {
                if (bref_cc == NULL) {
                    bref_cc_i = 0;
                    bref_cc = (uint8_t *)pzmalloc(sizeof(uint8_t) * 512);
                    memset(bref_cc, 0, sizeof(uint8_t) * 512);
                }

                uint32_t cc_sz = strlen((char *)(cc->d));
                memcpy(&bref_cc[bref_cc_i], cc->d, cc_sz);
                bref_cc_i += cc_sz;

                if (bref_cc_i == 5) {
                    bref_len = bref_cc[0] * 64 + bref_cc[1] * 16 + bref_cc[2] * 4 + bref_cc[3];

                    free(bref_cc);
                    bref_cc = NULL;

                    d_ref_list_t *ref = pzbinutil_DRefList_ReelFromRight(bref_q_tail, bref_count);

                    for (size_t j = 0; j < bref_len; j++) {
                        __insert_into(ret, *(ref->d), &retsz, &retsz_in_use);

                        pzbinutil_DRefList_Append(bref_q_tail, ref->d);
                        ref = ref->n;
                        pzbinutil_DRefList_DelLeft(bref_q_head);
                    }

                    bref_stage = 0;
                    bref_cc_i = 0;

                    bref_len = 0;
                    bref_count = 0;
                }
            } else {
                debug("Error: invalid [BACKREF] stage: %d", bref_stage);
            }

            continue;
        }

        if (match_sym(inst, "[EOA]", cc->d)) {
            break;
        }
    }
}
