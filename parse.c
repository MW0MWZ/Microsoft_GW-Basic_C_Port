/*
 * parse.c - Program line storage and management
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/*
 * Find a program line by line number
 */
line_t *
find_line(linenum)
int linenum;
{
    unsigned char *p;
    line_t *line;

    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;
        if (line->linenum == linenum) {
            return line;
        }
        if (line->linenum > linenum) {
            return NULL; /* Not found */
        }
        p += line->len;
    }
    return NULL;
}

/*
 * Insert or replace a program line
 */
void
insert_line(linenum, tokens, toklen)
int linenum;
unsigned char *tokens;
int toklen;
{
    unsigned char *p;
    unsigned char *insert_pos;
    line_t *line;
    line_t *newline;
    int oldlen;
    int newlen;
    long movesize;  /* Use long for 16-bit overflow protection */

    /* Find insertion position */
    insert_pos = NULL;
    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;
        if (line->linenum == linenum) {
            /* Replace existing line */
            oldlen = line->len;
            newlen = sizeof(line_t) + toklen - 1;
            /* Round up to even for PDP-11 word alignment */
            newlen = (newlen + 1) & ~1;

            if (newlen != oldlen) {
                /* Need to move memory - use long for size calc */
                movesize = (long)(g_state->vartab - p) - (long)oldlen;
                if (newlen > oldlen) {
                    /* Moving later in memory */
                    memmove(p + newlen, p + oldlen, (size_t)movesize);
                    g_state->vartab += (newlen - oldlen);
                } else {
                    /* Moving earlier in memory */
                    memmove(p + newlen, p + oldlen, (size_t)movesize);
                    g_state->vartab -= (oldlen - newlen);
                }
            }

            /* Update line header */
            line->len = newlen;
            memcpy(line->text, tokens, toklen);
            return;
        }
        if (line->linenum > linenum) {
            insert_pos = p;
            break;
        }
        p += line->len;
    }

    /* Insert new line */
    if (insert_pos == NULL) {
        insert_pos = p; /* Insert at end */
    }

    newlen = sizeof(line_t) + toklen - 1;
    /* Round up to even for PDP-11 word alignment */
    newlen = (newlen + 1) & ~1;

    /* Make room for new line - use long for size calc */
    movesize = (long)(g_state->vartab - insert_pos);
    memmove(insert_pos + newlen, insert_pos, (size_t)movesize);
    g_state->vartab += newlen;

    /* Create new line */
    newline = (line_t *)insert_pos;
    newline->linenum = linenum;
    newline->len = newlen;
    memcpy(newline->text, tokens, toklen);

    /* Clear variables after modifying program */
    clear_variables();
    clear_arrays();
}

/*
 * Delete a program line
 */
void
delete_line(linenum)
int linenum;
{
    unsigned char *p;
    line_t *line;
    int oldlen;
    long movesize;  /* Use long for 16-bit overflow protection */

    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;
        if (line->linenum == linenum) {
            /* Delete this line - use long for size calc */
            oldlen = line->len;
            movesize = (long)(g_state->vartab - (p + oldlen));
            memmove(p, p + oldlen, (size_t)movesize);
            g_state->vartab -= oldlen;

            /* Clear variables after modifying program */
            clear_variables();
            clear_arrays();
            return;
        }
        p += line->len;
    }
}

/*
 * List program lines
 */
void
list_program(start, end)
int start;
int end;
{
    unsigned char *p;
    line_t *line;
    char *text;

    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum >= start && line->linenum <= end) {
            text = detokenize_line(line->text);
            if (text) {
                printf("%d %s\n", line->linenum, text);
                free(text);
            }
        }

        if (line->linenum > end) {
            break;
        }

        p += line->len;
    }
}

/*
 * Clear program and variables
 */
void
new_program()
{
    /* Mark end of program */
    g_state->txttab[0] = 0;
    g_state->txttab[1] = 0;

    /* Reset memory pointers - vartab points AFTER the end marker */
    g_state->vartab = g_state->txttab + 2;
    g_state->arytab = g_state->txttab + 2;
    g_state->strend = g_state->txttab + 2;

    /* Clear variables and arrays */
    clear_variables();
    clear_arrays();

    /* Reset execution state */
    g_state->curlin = 0;
    g_state->txtptr = NULL;
    g_state->running = 0;
    g_state->forsp = 0;
    g_state->gosubsp = 0;
    g_state->whilesp = 0;
    g_state->datlin = 0;
    g_state->datptr = NULL;
}
