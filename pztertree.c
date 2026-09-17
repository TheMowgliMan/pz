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

defer: // The evil GOTO, although probably less evil than that "for" statement down there
    for (tq_t *cleanup = head; cleanup;) {
        tq_t *temp = cleanup->n;

        pzfree(cleanup);
        cleanup = temp;
    }

    return;
}

pzbint_ret_t pztertree_Get(pzbint_t *head, uint8_t *key) {
    pzbinti_t *tree = head->h;
    for (uint8_t i = 0, v = key[i]; key[i] != 0; i++, v = key[i]) { // I love how diabolical C can be
        if (!v) {
            pzbint_ret_t ret;
            ret.d = tree->v;
            ret.dlen = tree->vlen;

            return ret;
        } else {
            if (v == 1) {
                tree = tree->n_l;
            } else if (v == 2) {
                tree = tree->n_r;
            } else {
                tree = tree->n_c;
            }
        }
    }

    pzbint_ret_t ret;
    ret.d = NULL;
    ret.dlen = 0;

    return ret;
}
