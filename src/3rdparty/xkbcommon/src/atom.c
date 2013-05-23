/***********************************************************
 * Copyright 1987, 1998  The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group.
 *
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 ******************************************************************/

/************************************************************
 * Copyright 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#include "utils.h"
#include "atom.h"

struct atom_node {
    struct atom_node *left, *right;
    xkb_atom_t atom;
    unsigned int fingerprint;
    char *string;
};

struct atom_table {
    struct atom_node *root;
    darray(struct atom_node *) table;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table;

    table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->table);
    darray_growalloc(table->table, 100);
    darray_append(table->table, NULL);

    return table;
}

static void
free_atom(struct atom_node *patom)
{
    if (!patom)
        return;

    free_atom(patom->left);
    free_atom(patom->right);
    free(patom->string);
    free(patom);
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    free_atom(table->root);
    darray_free(table->table);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    if (atom >= darray_size(table->table) ||
        darray_item(table->table, atom) == NULL)
        return NULL;

    return darray_item(table->table, atom)->string;
}

char *
atom_strdup(struct atom_table *table, xkb_atom_t atom)
{
    return strdup_safe(atom_text(table, atom));
}

static bool
find_node_pointer(struct atom_table *table, const char *string,
                  struct atom_node ***np_out, unsigned int *fingerprint_out)
{
    struct atom_node **np;
    unsigned i;
    int comp;
    unsigned int fp = 0;
    size_t len;
    bool found = false;

    len = strlen(string);
    np = &table->root;
    for (i = 0; i < (len + 1) / 2; i++) {
        fp = fp * 27 + string[i];
        fp = fp * 27 + string[len - 1 - i];
    }

    while (*np) {
        if (fp < (*np)->fingerprint) {
            np = &((*np)->left);
        }
        else if (fp > (*np)->fingerprint) {
            np = &((*np)->right);
        }
        else {
            /* now start testing the strings */
            comp = strncmp(string, (*np)->string, len);
            if (comp < 0 || (comp == 0 && len < strlen((*np)->string))) {
                np = &((*np)->left);
            }
            else if (comp > 0) {
                np = &((*np)->right);
            }
            else {
                found = true;
                break;
            }
        }
    }

    *fingerprint_out = fp;
    *np_out = np;
    return found;
}

xkb_atom_t
atom_lookup(struct atom_table *table, const char *string)
{
    struct atom_node **np;
    unsigned int fp;

    if (!string)
        return XKB_ATOM_NONE;

    if (!find_node_pointer(table, string, &np, &fp))
        return XKB_ATOM_NONE;

    return (*np)->atom;
}

/*
 * If steal is true, we do not strdup @string; therefore it must be
 * dynamically allocated, not be free'd by the caller and not be used
 * afterwards. Use to avoid some redundant allocations.
 */
xkb_atom_t
atom_intern(struct atom_table *table, const char *string,
            bool steal)
{
    struct atom_node **np;
    struct atom_node *nd;
    unsigned int fp;

    if (!string)
        return XKB_ATOM_NONE;

    if (find_node_pointer(table, string, &np, &fp)) {
        if (steal)
            free(UNCONSTIFY(string));
        return (*np)->atom;
    }

    nd = malloc(sizeof(*nd));
    if (!nd)
        return XKB_ATOM_NONE;

    if (steal) {
        nd->string = UNCONSTIFY(string);
    }
    else {
        nd->string = strdup(string);
        if (!nd->string) {
            free(nd);
            return XKB_ATOM_NONE;
        }
    }

    *np = nd;
    nd->left = nd->right = NULL;
    nd->fingerprint = fp;
    nd->atom = darray_size(table->table);
    darray_append(table->table, nd);

    return nd->atom;
}
