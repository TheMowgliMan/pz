#include "pzcompressor.h"

#include "pzbinutil.h"
#include "pztertree.h"
#include "macros.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pccompressor_ImportTreeFile(pz_comp_inst_t *inst, char *tree_string, uint32_t tree_size) {
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
    pccompressor_ImportTreeFile(inst, tree_string, tree_size);

    uint8_t *bin = &fdata[tree_size + 4];
    ref_list_t *data = pzbinutil_FromBinary(bin, fdatalen - (tree_size + 4));

    uint8_t *ret = (uint8_t *)pzmalloc(sizeof(uint8_t) * DECOMPRESS_OVER_ALLOCATE_SIZE);
    size_t retsz = DECOMPRESS_OVER_ALLOCATE_SIZE;
    size_t retsz_in_use = 0;

    bool is_meta = false;
    int pz_protocol_ver = -1;

    uint8_t reptstage = 0;
    size_t reptcount = 0;
    uint8_t *reptcc;

    d_ref_list_t *bref_q_head = (d_ref_list_t *)pzmalloc(sizeof(d_ref_list_t));
    bref_q_head->n = NULL;
    bref_q_head->p = NULL;
    d_ref_list_t *bref_q_tail = bref_q_head;
    size_t bref_q_sz = 0;

    uint8_t bref_stage = 0;
    uint8_t *bref_cc;

    size_t bref_len = 0;
    size_t bref_count = 0;

    uint64_t bref_char_passed = 0;

    // TODO: decompression loop
}
