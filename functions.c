/*
 * functions.c - Built-in BASIC functions
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/*
 * SGN function - sign of a number
 */
double
fn_sgn(x)
double x;
{
    if (x > 0.0) return 1.0;
    if (x < 0.0) return -1.0;
    return 0.0;
}

/*
 * INT function - integer part
 */
double
fn_int(x)
double x;
{
    return floor(x);
}

/*
 * ABS function - absolute value
 */
double
fn_abs(x)
double x;
{
    return fabs(x);
}

/*
 * SQR function - square root
 */
double
fn_sqr(x)
double x;
{
    if (x < 0.0) {
        error(ERR_ILLEGAL_FUNC);
        return 0.0;
    }
    return sqrt(x);
}

/*
 * RND function - random number
 */
double
fn_rnd(x)
double x;
{
    if (x < 0.0) {
        /* Reseed with specific value */
        g_state->rndseed = (unsigned long)(-x * 1000000.0);
    } else if (x == 0.0) {
        /* Return last random number */
    } else {
        /* Generate new random number */
        g_state->rndseed = (g_state->rndseed * 1103515245 + 12345) & 0x7fffffff;
    }

    return (double)g_state->rndseed / 2147483648.0;
}

/*
 * SIN function
 */
double
fn_sin(x)
double x;
{
    return sin(x);
}

/*
 * COS function
 */
double
fn_cos(x)
double x;
{
    return cos(x);
}

/*
 * TAN function
 */
double
fn_tan(x)
double x;
{
    return tan(x);
}

/*
 * ATN function - arctangent
 */
double
fn_atn(x)
double x;
{
    return atan(x);
}

/*
 * LOG function - natural logarithm
 */
double
fn_log(x)
double x;
{
    if (x <= 0.0) {
        error(ERR_ILLEGAL_FUNC);
        return 0.0;
    }
    return log(x);
}

/*
 * EXP function - e^x
 */
double
fn_exp(x)
double x;
{
    double result;

    result = exp(x);
    if (result == HUGE_VAL) {
        error(ERR_OVERFLOW);
        return 0.0;
    }
    return result;
}

/*
 * LEN function - string length
 */
int
fn_len(s)
string_t *s;
{
    if (!s) {
        return 0;
    }
    return s->len;
}

/*
 * ASC function - ASCII code of first character
 */
int
fn_asc(s)
string_t *s;
{
    if (!s || s->len == 0) {
        error(ERR_ILLEGAL_FUNC);
        return 0;
    }
    return (unsigned char)s->ptr[0];
}

/*
 * CHR$ function - character from ASCII code
 */
string_t *
fn_chr(n)
int n;
{
    string_t *result;
    char c;

    if (n < 0 || n > 255) {
        error(ERR_ILLEGAL_FUNC);
        return alloc_string(0);
    }

    result = alloc_string(1);
    if (!result) {
        return NULL;
    }

    c = (char)n;
    result->ptr[0] = c;
    result->ptr[1] = '\0';

    return result;
}

/*
 * STR$ function - convert number to string
 */
string_t *
fn_str(x)
double x;
{
    char buf[40];
    int len;

    /* Format number */
    if (x == (double)(int)x) {
        sprintf(buf, "%d", (int)x);
    } else {
        sprintf(buf, "%g", x);
    }

    /* Add leading space for positive numbers */
    len = strlen(buf);
    if (buf[0] != '-') {
        memmove(buf + 1, buf, len + 1);
        buf[0] = ' ';
    }

    return string_from_cstr(buf);
}

/*
 * VAL function - convert string to number
 */
double
fn_val(s)
string_t *s;
{
    char *cstr;
    double result;

    if (!s || s->len == 0) {
        return 0.0;
    }

    cstr = string_to_cstr(s);
    if (!cstr) {
        return 0.0;
    }

    result = atof(cstr);
    free(cstr);

    return result;
}

/*
 * LEFT$ function - leftmost n characters
 */
string_t *
fn_left(s, n)
string_t *s;
int n;
{
    string_t *result;

    if (!s || n <= 0) {
        return alloc_string(0);
    }

    if (n > s->len) {
        n = s->len;
    }

    result = alloc_string(n);
    if (!result) {
        return NULL;
    }

    if (s->ptr) {
        memcpy(result->ptr, s->ptr, n);
        result->ptr[n] = '\0';
    }

    return result;
}

/*
 * RIGHT$ function - rightmost n characters
 */
string_t *
fn_right(s, n)
string_t *s;
int n;
{
    string_t *result;
    int start;

    if (!s || n <= 0) {
        return alloc_string(0);
    }

    if (n > s->len) {
        n = s->len;
    }

    start = s->len - n;

    result = alloc_string(n);
    if (!result) {
        return NULL;
    }

    if (s->ptr) {
        memcpy(result->ptr, s->ptr + start, n);
        result->ptr[n] = '\0';
    }

    return result;
}

/*
 * MID$ function - substring
 */
string_t *
fn_mid(s, start, len)
string_t *s;
int start;
int len;
{
    string_t *result;
    int actual_start;
    int actual_len;

    if (!s || start < 1) {
        return alloc_string(0);
    }

    /* BASIC uses 1-based indexing */
    actual_start = start - 1;

    if (actual_start >= s->len) {
        return alloc_string(0);
    }

    actual_len = len;
    if (actual_start + actual_len > s->len) {
        actual_len = s->len - actual_start;
    }

    if (actual_len <= 0) {
        return alloc_string(0);
    }

    result = alloc_string(actual_len);
    if (!result) {
        return NULL;
    }

    if (s->ptr) {
        memcpy(result->ptr, s->ptr + actual_start, actual_len);
        result->ptr[actual_len] = '\0';
    }

    return result;
}

/*
 * FRE function - free memory
 * FRE(0) returns bytes free for program/variables
 * FRE("") returns bytes free for strings (same in this implementation)
 */
double
fn_fre(x)
double x;
{
    long freemem;

    /* Argument is ignored - use it to suppress warning */
    if (x) { /* do nothing */ }
    freemem = (long)(g_state->fretop - g_state->strend);
    return (double)freemem;
}

/*
 * INSTR function - find substring
 */
int
fn_instr(start, s1, s2)
int start;
string_t *s1;
string_t *s2;
{
    int i;
    int j;
    int match;

    if (!s1 || !s2 || start < 1) {
        return 0;
    }

    if (s2->len == 0) {
        return start;
    }

    if (start > s1->len) {
        return 0;
    }

    /* Search for s2 in s1 starting at position start */
    for (i = start - 1; i <= s1->len - s2->len; i++) {
        match = 1;
        for (j = 0; j < s2->len; j++) {
            if (s1->ptr[i + j] != s2->ptr[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            return i + 1; /* Return 1-based position */
        }
    }

    return 0; /* Not found */
}
