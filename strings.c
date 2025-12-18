/*
 * strings.c - String memory management
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/*
 * Allocate a string of given length
 */
string_t *
alloc_string(len)
int len;
{
    string_t *str;

    str = (string_t *)malloc(sizeof(string_t));
    if (!str) {
        error(ERR_OUT_OF_STR);
        return NULL;
    }

    if (len > 0) {
        str->ptr = (char *)malloc(len + 1);
        if (!str->ptr) {
            free(str);
            error(ERR_OUT_OF_STR);
            return NULL;
        }
        str->ptr[len] = '\0';
    } else {
        str->ptr = NULL;
    }

    str->len = len;
    return str;
}

/*
 * Free a string
 */
void
free_string(str)
string_t *str;
{
    if (str) {
        if (str->ptr) {
            free(str->ptr);
        }
        free(str);
    }
}

/*
 * Copy a string
 */
string_t *
copy_string(src)
string_t *src;
{
    string_t *dest;

    if (!src) {
        return alloc_string(0);
    }

    dest = alloc_string(src->len);
    if (!dest) {
        return NULL;
    }

    if (src->len > 0 && src->ptr) {
        memcpy(dest->ptr, src->ptr, src->len);
        dest->ptr[src->len] = '\0';
    }

    return dest;
}

/*
 * Concatenate two strings
 */
string_t *
concat_strings(s1, s2)
string_t *s1;
string_t *s2;
{
    string_t *result;
    int newlen;

    if (!s1 || !s2) {
        return alloc_string(0);
    }

    newlen = s1->len + s2->len;
    if (newlen > 255) {
        error(ERR_STRING_LONG);
        return NULL;
    }

    result = alloc_string(newlen);
    if (!result) {
        return NULL;
    }

    if (s1->len > 0 && s1->ptr) {
        memcpy(result->ptr, s1->ptr, s1->len);
    }
    if (s2->len > 0 && s2->ptr) {
        memcpy(result->ptr + s1->len, s2->ptr, s2->len);
    }
    result->ptr[newlen] = '\0';

    return result;
}

/*
 * Compare two strings
 * Returns: <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
int
compare_strings(s1, s2)
string_t *s1;
string_t *s2;
{
    int minlen;
    int result;

    if (!s1 && !s2) {
        return 0;
    }
    if (!s1) {
        return -1;
    }
    if (!s2) {
        return 1;
    }

    minlen = s1->len < s2->len ? s1->len : s2->len;

    if (s1->ptr && s2->ptr && minlen > 0) {
        result = memcmp(s1->ptr, s2->ptr, minlen);
        if (result != 0) {
            return result;
        }
    }

    /* If prefixes match, shorter string is less */
    return s1->len - s2->len;
}

/*
 * Create string from C string
 */
string_t *
string_from_cstr(cstr)
const char *cstr;
{
    string_t *str;
    int len;

    if (!cstr) {
        return alloc_string(0);
    }

    len = strlen(cstr);
    if (len > 255) {
        len = 255;
    }

    str = alloc_string(len);
    if (!str) {
        return NULL;
    }

    if (len > 0) {
        memcpy(str->ptr, cstr, len);
        str->ptr[len] = '\0';
    }

    return str;
}

/*
 * Convert string to C string (caller must free)
 */
char *
string_to_cstr(str)
string_t *str;
{
    char *cstr;

    if (!str || str->len == 0) {
        cstr = (char *)malloc(1);
        if (cstr) {
            cstr[0] = '\0';
        }
        return cstr;
    }

    cstr = (char *)malloc(str->len + 1);
    if (!cstr) {
        return NULL;
    }

    memcpy(cstr, str->ptr, str->len);
    cstr[str->len] = '\0';

    return cstr;
}
