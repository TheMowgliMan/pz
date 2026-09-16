#include "pztertree.h"

#include "macros.h"

#include <string.h>

pzbint_t *pztertree_New(void) {
    debug("Creating ternary tree...");

    pzbint_t *r = (pzbint_t *)pzmalloc(sizeof(pzbint_t));
    r->h = NULL;
    return r;
}

void pztertree_Add(pzbint_t *tree, void *d, uint8_t dlen) {
    /* Never forget that C has manual memory management... */
    void *temp = (void *)pzmalloc(dlen);
    memcpy(temp, d, dlen);
    d = temp;

    if (tree->h == NULL) {
        tree->h = (pzbinti_t *)pzmalloc(sizeof(pzbinti_t));

        tree->h->n_l = NULL;
        tree->h->n_r = NULL;
        tree->h->n_c = NULL;

        tree->h->v = d;
        tree->h->vlen = dlen;

        return;
    }

    pzbinti_t *item = (pzbinti_t *)pzmalloc(sizeof(pzbinti_t));
    item->n_l = NULL;
    item->n_r = NULL;
    item->n_c = NULL;

    item->v = d;
    item->vlen = dlen;

    tq_t *head = (tq_t *)pzmalloc(sizeof(tq_t));
    head->i = tree->h;
    head->n = NULL;

    tq_t *q = head;
    for (;;) {
        if (q->i->n_l) {
            tq_t *t = (tq_t *)pzmalloc(sizeof(tq_t));
            t->n = NULL;

            t->i = q->i->n_l;
            q->n = t;
        } else {
            q->i->n_l = item;
            goto defer;
        }

        if (q->i->n_r) {
            tq_t *t = (tq_t *)pzmalloc(sizeof(tq_t));
            t->n = NULL;

            t->i = q->i->n_r;
            q->n = t;
        } else {
            q->i->n_r = item;
            goto defer;
        }

        if (q->i->n_c) {
            tq_t *t = (tq_t *)pzmalloc(sizeof(tq_t));
            t->n = NULL;

            t->i = q->i->n_c;
            q->n = t;
        } else {
            q->i->n_c = item;
            goto defer;
        }
    }

defer: // The evil GOTO, although probably less evil than that for statement down there
    for (tq_t *cleanup = head; cleanup;) {
        tq_t *temp = cleanup->n;
        free(cleanup);
        cleanup = temp;
    }

    return;
}
