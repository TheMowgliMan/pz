#include "pzcompressor.h"

#include "pztertree.h"
#include "macros.h"

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
    }
}

uint8_t *pzcompressor_DecompressFile(pz_comp_inst_t *inst, uint8_t *fdata) {
    uint32_t tree_size = 0;

    tree_size |= fdata[3];
    tree_size |= fdata[2] << 8;
    tree_size |= fdata[1] << 16;
    tree_size |= fdata[0] << 24;

    tree_size += 4;
    char *tree_string = (char *)(&fdata[4]);

    /* FIXME: previous tree and symbols will be memory leaked */
    pccompressor_ImportTreeFile(inst, tree_string, tree_size); // TODO: IMPLEMENT!
}
