#include "pzcompressor.h"

#include "pzbinutil.h"
#include "pztertree.h"
#include "macros.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void __insert_into(uint8_t **ptr, uint8_t ins, size_t *ptrsz, size_t *ptrsz_in_use) {
    if (*ptrsz_in_use < *ptrsz) {
        (*ptr)[*ptrsz_in_use] = ins;
        (*ptrsz_in_use)++;
    } else {
        uint8_t *t = (uint8_t *)realloc(*ptr, sizeof(uint8_t) * (*ptrsz + DECOMPRESS_OVER_ALLOCATE_SIZE));
        if (t != NULL) {
            *ptr = t;
            (*ptr)[*ptrsz_in_use] = ins;
            (*ptrsz_in_use)++;
            (*ptrsz) += DECOMPRESS_OVER_ALLOCATE_SIZE;
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

bool is_sym(pz_comp_inst_t *inst, uint8_t *sym_key) {
    for (uint16_t i = 0; i < MAXIMUM_SYMBOL_COUNT; i++) {
        if ((size_t)(inst->symbols[i] == 0))
            continue;

        if (strcmp((char *)(inst->symbol_references[i]), (char *)sym_key) == 0)
            return true;
    }

    return false;
}

void pzcompressor_GenerateTree(pz_comp_inst_t *inst, uint8_t *data, size_t data_sz) {
    if (inst->tree == NULL) {
        inst->tree = (pzbint_t *)pzmalloc(sizeof(pzbint_t));
        inst->tree->h = NULL;
    }

    uint8_t res_count_k[256];
    uint64_t res_count_v[256];

    {
        uint8_t count_arr_k[256];
        for (uint16_t i = 0; i < 256; i++) {
            count_arr_k[i] = i;
        }

        uint64_t count_arr_v[256];
        for (uint64_t i = 0; i < data_sz; i++) {
            count_arr_v[data[i]]++;
        }

        for (uint16_t i = 0; i < 256; i++) {
            uint8_t lk = 0;
            uint64_t lv = 0;
            for (uint16_t k = 0; k < 256; k++) {
                if (count_arr_v[k] > lv) {
                    lv = count_arr_v[k];
                    lk = count_arr_k[k];
                }
            }

            res_count_v[i] = lv;
            res_count_k[i] = lk;

            count_arr_v[i] = 0;
        }

        /* We should now have a nice sorted list of item commonnesses! */
    }

    bool symbols_not_added = true;

    for (size_t i = 0; i < 256; i++) {
        if (((double)(res_count_v[i]) < ((double)(res_count_v[0]) / 100.0)) && symbols_not_added) {
            symbols_not_added = false;

            char c[] = "[REPT]";
            pztertree_Add(inst->tree, &c, strlen(c) + 1);
            char c2[] = "[BACKREF]";
            pztertree_Add(inst->tree, &c2, strlen(c2) + 1);
            char c3[] = "[PZ META]";
            pztertree_Add(inst->tree, &c3, strlen(c3) + 1);
            char c4[] = "[END META]";
            pztertree_Add(inst->tree, &c4, strlen(c4) + 1);
            char c5[] = "[PZ PROTOCOL 0]";
            pztertree_Add(inst->tree, &c5, strlen(c5) + 1);
        }

        pztertree_Add(inst->tree, &res_count_k[i], sizeof(uint8_t));
    }

    if (symbols_not_added) {
        char c[] = "[REPT]";
        pztertree_Add(inst->tree, &c, strlen(c) + 1);
        char c2[] = "[BACKREF]";
        pztertree_Add(inst->tree, &c2, strlen(c2) + 1);
        char c3[] = "[PZ META]";
        pztertree_Add(inst->tree, &c3, strlen(c3) + 1);
        char c4[] = "[END META]";
        pztertree_Add(inst->tree, &c4, strlen(c4) + 1);
        char c5[] = "[PZ PROTOCOL 0]";
        pztertree_Add(inst->tree, &c5, strlen(c5) + 1);
    }

    char c[] = "[EOA]";
    pztertree_Add(inst->tree, &c, strlen(c) + 1);
}

uint8_t pzcompressor_CompressFile(pz_comp_inst_t *inst, uint8_t *data, size_t data_sz) {
    pzcompressor_GenerateTree(inst, data, data_sz);

    // TODO: More code here!
}

void pzcompressor_ImportTreeFile(pz_comp_inst_t *inst, char *tree_string, uint32_t tree_size) {
    inst->tree = pztertree_New();

    printf("Allocating symbols\n");
    inst->symbols = (char**)pzmalloc(sizeof(char *) * MAXIMUM_SYMBOL_COUNT);
    memset(inst->symbols, 0, sizeof(char *) * MAXIMUM_SYMBOL_COUNT);

    inst->symbol_references = (uint8_t**)pzmalloc(sizeof(uint8_t *) * MAXIMUM_SYMBOL_COUNT);
    memset(inst->symbol_references, 0, sizeof(uint8_t *) * MAXIMUM_SYMBOL_COUNT);

    inst->used_symbols = 0;

    printf("Allocating string...\n");
    char *string = (char *)pzmalloc(sizeof(char) * (tree_size + 1));
    memcpy(string, tree_string, tree_size);
    string[tree_size] = '\0';

    printf("Iterating...\n");

    for (char *ln = strtok(string, "\n"); ln != NULL; ln = strtok(NULL, "\n")) {
        printf("%s\n", ln);

        char f = ln[0];
        if (f >= '0' && f <= '9') {
            printf("DIGIT\n");
            int d = atoi(ln);

            char *dc = (char *)pzmalloc(sizeof(char));
            *dc = d & 0xff;

            pztertree_Add(inst->tree, dc, sizeof(char));

            uint8_t *sym_key = pztertree_InOrderFind(inst->tree, dc, sizeof(char));
            printf("KEY: ");
            for (int i = 0; i < strlen((char *)sym_key) + 1; i++) {
                printf("%d", sym_key[i]);
            }
            printf("\n");
        } else {
            pztertree_Add(inst->tree, ln, sizeof(char) * (1 + strlen(ln)));

            uint8_t *sym_key = pztertree_InOrderFind(inst->tree, ln, strlen(ln) + 1);
            printf("KEY: ");
            for (int i = 0; i < strlen((char *)sym_key) + 1; i++) {
                printf("%d", sym_key[i]);
            }
            printf("\n");
        }

        if (f == '[') {
            printf("SYMBOL\n");
            inst->symbols[inst->used_symbols] = (char *)pzmalloc(sizeof(char) * (strlen(ln) + 1));
            printf("2\n");
            memcpy(inst->symbols[inst->used_symbols], ln, sizeof(char) * (1 + strlen(ln)));
            printf("3\n");

            uint8_t *sym_key = pztertree_InOrderFind(inst->tree, ln, strlen(ln) + 1);
            printf("%p\n", sym_key);
            assertif(sym_key == NULL);
            printf("4\n");
            printf("%d\n", inst->used_symbols);
            inst->symbol_references[inst->used_symbols] = sym_key;
            printf("5\n");

            inst->used_symbols++;
        }
    }

    pzfree(string);
}

uint8_t *pzcompressor_DecompressFile(pz_comp_inst_t *inst, uint8_t *fdata, size_t fdatalen, size_t *fsz_ret) {
    printf("Arranging tree...\n");
    uint32_t tree_size = 0;

    tree_size |= fdata[3];
    tree_size |= fdata[2] << 8;
    tree_size |= fdata[1] << 16;
    tree_size |= fdata[0] << 24;

    // tree_size += 4;
    char *tree_string = (char *)(&fdata[4]);

    /* FIXME: previous tree and symbols will be memory leaked */
    pzcompressor_ImportTreeFile(inst, tree_string, tree_size);

    printf("Debinarizing...\n");
    uint8_t *bin = &fdata[tree_size + 4];
    ref_list_t *data = pzbinutil_FromBinary(bin, fdatalen - (tree_size + 4));

    printf("Allocating variables (return)...\n");
    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * DECOMPRESS_OVER_ALLOCATE_SIZE);
    size_t retsz = DECOMPRESS_OVER_ALLOCATE_SIZE;
    size_t retsz_in_use = 0;

    printf("Allocating variables (metadata)...\n");
    bool is_meta = false;
    int pz_protocol_ver = -1;

    printf("Allocating variables (rept)...\n");
    uint8_t reptstage = 0;
    size_t reptcount = 0;
    uint8_t *reptcc = NULL;
    uint16_t reptcc_i = 0;

    printf("Allocating variables (backref queue)...\n");
    d_ref_list_t *bref_q_head = (d_ref_list_t *)pzmalloc(sizeof(d_ref_list_t));
    bref_q_head->n = NULL;
    bref_q_head->p = NULL;
    d_ref_list_t *bref_q_tail = bref_q_head;
    size_t bref_q_sz = 0;

    {
        char m = (char)(*((uint8_t *)(inst->tree->h->v)));
        for (uint64_t i = 0; i < 65536; i++) { // Ugly...
            pzbinutil_DRefList_Append(&bref_q_tail, (uint8_t *)(pztertree_InOrderFind(inst->tree, &m, sizeof(char))));
        }
    }

    printf("Allocating variables (backref)...\n");
    uint8_t bref_stage = 0;
    uint8_t *bref_cc = NULL;
    uint16_t bref_cc_i = 0;

    size_t bref_len = 0;
    size_t bref_count = 0;

    uint64_t bref_char_passed = 0;

    printf("Entering loop...\n");
    for (ref_list_t *cc = data; cc != NULL; cc = cc->n) {
        while (bref_q_sz > 65536) {
            pzbinutil_DRefList_DelLeft(&bref_q_head);
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
                    __insert_into(&ret, *((uint8_t *)(c.d)), &retsz, &retsz_in_use); // This modifies the variables in-place for me

                    if (bref_q_sz > 65536)
                        pzbinutil_DRefList_DelLeft(&bref_q_head);

                    pzbinutil_DRefList_Append(&bref_q_tail, (uint8_t *)(c.d));

                    bref_char_passed++;

                    (*fsz_ret)++;
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

                if (cc->d == NULL) {
                    printf("NULL found, continuing... (%d)\n", bref_cc_i);
                    continue;
                }

                uint32_t cc_sz = strlen((char *)(cc->d)) + 1;
                memcpy(&bref_cc[bref_cc_i], cc->d, cc_sz);
                bref_cc_i += cc_sz;

                if (bref_cc_i == 5) {
                    bref_len = bref_cc[0] * 64 + bref_cc[1] * 16 + bref_cc[2] * 4 + bref_cc[3];

                    free(bref_cc);
                    bref_cc = NULL;

                    d_ref_list_t *ref = pzbinutil_DRefList_ReelFromRight(bref_q_tail, bref_count - 1);

                    for (size_t j = 0; j < bref_len; j++) {
                        __insert_into(&ret, *(ref->d), &retsz, &retsz_in_use);

                        pzbinutil_DRefList_Append(&bref_q_tail, ref->d);
                        ref = ref->n;

                        if (bref_q_sz > 65536)
                            pzbinutil_DRefList_DelLeft(&bref_q_head);

                        (*fsz_ret)++;
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

        if (match_sym(inst, "[PZ META]", cc->d)) {
            is_meta = true;
        } else if (match_sym(inst, "[END META]", cc->d)) {
            is_meta = false;

            if (pz_protocol_ver == -1) {
                debug("Decompressing files with no stated protocol is deprecated and may be removed at any time!");
                return NULL;
            }
        }

        if (match_sym(inst, "[REPT]", cc->d))
            reptstage = 1;
        else if (match_sym(inst, "[BACKREF]", cc->d)) {
            printf("BACKREF!\n");
            bref_stage = 1;
            bref_char_passed = 0;
        }

        if (!is_sym(inst, cc->d) && !is_meta) {
            pzbint_ret_t c = pztertree_Get(inst->tree, cc->d);
            __insert_into(&ret, *((uint8_t *)(c.d)), &retsz, &retsz_in_use);

            pzbinutil_DRefList_Append(&bref_q_tail, ((uint8_t *)(c.d)));
            pzbinutil_DRefList_DelLeft(&bref_q_head);

            bref_char_passed++;
            (*fsz_ret)++;
        }

        if (is_meta) {
            if (match_sym(inst, "[PZ PROTOCOL 0]", cc->d)) {
                pz_protocol_ver = 0;
            }
        }
    }

    printf("Done!\n");

    return ret; // FIXME: I guarantee there are a bajillion memory leaks
}

#ifdef COMPRESSOR_TEST
int main(int argc, char *argv[]) {
    printf("Started\n");

    FILE *f = fopen("small.txt.pz", "rb");
    if (f == NULL) {
        debug("File not found");
        return -1;
    }

    fseek(f, 0, SEEK_END);

    size_t fsz = (size_t)ftell(f);
    rewind(f);

    uint8_t *buf = (uint8_t *)pzmalloc(fsz);
    fread(buf, fsz, 1, f);

    fclose(f);

    pz_comp_inst_t *comp = (pz_comp_inst_t *)pzmalloc(sizeof(pz_comp_inst_t));

    size_t fsz_ret = 0;
    uint8_t *decompressed = pzcompressor_DecompressFile(comp, buf, fsz, &fsz_ret);

    printf("%s\n", (char *)decompressed);

    f = fopen("smallc.txt", "wb");
    fwrite(decompressed, sizeof(uint8_t) * fsz_ret, 1, f);
    fclose(f);

    return 0;
}
#endif
