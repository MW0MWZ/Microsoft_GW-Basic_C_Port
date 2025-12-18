/*
 * arrays.c - Array storage and management
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* K&R C compatible character tests - ctype macros may fail on old systems */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/*
 * Find an array by name, optionally create if not found
 */
array_t *
find_array(name, create)
const char *name;
int create;
{
    array_t *arr;
    array_t *prev;
    char normname[NAMLEN+1];
    const char *s;
    char *d;
    int type;
    int len;

    /* Normalize name (uppercase, no type suffix for now) */
    d = normname;
    s = name;
    len = 0;
    type = TYPE_SNG; /* Default type */

    while (*s && len < NAMLEN) {
        if (IS_ALNUM(*s) || *s == '.') {
            if (*s >= 'a' && *s <= 'z') {
                *d++ = *s - 'a' + 'A';
            } else {
                *d++ = *s;
            }
            len++;
        } else if (*s == '$' || *s == '%' || *s == '!' || *s == '#') {
            /* Type suffix */
            switch (*s) {
                case '$': type = TYPE_STR; break;
                case '%': type = TYPE_INT; break;
                case '!': type = TYPE_SNG; break;
                case '#': type = TYPE_DBL; break;
            }
            break;
        } else {
            break;
        }
        s++;
    }
    *d = '\0';

    /* Search array list */
    prev = NULL;
    for (arr = g_state->arrlist; arr != NULL; arr = arr->next) {
        if (strcmp(arr->name, normname) == 0) {
            return arr;
        }
        prev = arr;
    }

    /* Not found */
    if (!create) {
        return NULL;
    }

    /* Create new array (not dimensioned yet) */
    arr = (array_t *)malloc(sizeof(array_t));
    if (!arr) {
        error(ERR_OUT_OF_MEM);
        return NULL;
    }

    strcpy(arr->name, normname);
    arr->type = type;
    arr->ndims = 0;
    arr->size = 0;
    arr->data = NULL;
    arr->next = NULL;

    /* Add to list */
    if (prev) {
        prev->next = arr;
    } else {
        g_state->arrlist = arr;
    }

    return arr;
}

/*
 * Dimension an array
 */
void
dimension_array(name, dims, ndims, type)
const char *name;
int *dims;
int ndims;
int type;
{
    array_t *arr;
    int i;
    long size;  /* Use long to detect overflow on 16-bit systems */

    /* Check if array already exists */
    arr = find_array(name, 0);
    if (arr && arr->ndims > 0) {
        error(ERR_REDIM);
        return;
    }

    /* Create array if needed */
    if (!arr) {
        arr = find_array(name, 1);
        if (!arr) {
            return;
        }
    }

    /* Calculate total size using long to detect overflow */
    size = 1L;
    for (i = 0; i < ndims; i++) {
        arr->dims[i] = dims[i];
        size *= (long)dims[i];
        /* Check for overflow - limit to reasonable size for platform */
#if IS_16BIT
        /* PDP-11: ~16KB max for array data (2048 elements * 8 bytes) */
        if (size > 2048L) {
#else
        if (size > 16384L) {
#endif
            error(ERR_OUT_OF_MEM);
            return;
        }
    }

    arr->ndims = ndims;
    arr->size = (int)size;
    arr->type = type;

    /* Allocate array data */
    arr->data = (value_t *)calloc((size_t)size, sizeof(value_t));
    if (!arr->data) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Initialize string elements if string array */
    if (type == TYPE_STR) {
        for (i = 0; i < (int)size; i++) {
            arr->data[i].strval = alloc_string(0);
        }
    }
}

/*
 * Get pointer to array element
 */
value_t *
array_element(name, indices, nindices)
const char *name;
int *indices;
int nindices;
{
    array_t *arr;
    int i;
    long offset;      /* Use long for offset calculation */
    long multiplier;

    arr = find_array(name, 0);
    if (!arr) {
        error(ERR_SUBSCRIPT);
        return NULL;
    }

    /* Auto-dimension if not already dimensioned */
    if (arr->ndims == 0) {
        int autodims[8];
        for (i = 0; i < nindices; i++) {
            autodims[i] = 11; /* Default dimension is 0 to 10 */
        }
        dimension_array(name, autodims, nindices, arr->type);
    }

    /* Check dimension count */
    if (nindices != arr->ndims) {
        error(ERR_SUBSCRIPT);
        return NULL;
    }

    /* Calculate offset using long arithmetic */
    offset = 0L;
    multiplier = 1L;
    for (i = arr->ndims - 1; i >= 0; i--) {
        if (indices[i] < 0 || indices[i] >= arr->dims[i]) {
            error(ERR_SUBSCRIPT);
            return NULL;
        }
        offset += (long)indices[i] * multiplier;
        multiplier *= (long)arr->dims[i];
    }

    return &arr->data[offset];
}

/*
 * Clear all arrays
 */
void
clear_arrays()
{
    array_t *arr;
    array_t *next;
    int i;

    for (arr = g_state->arrlist; arr != NULL; arr = next) {
        next = arr->next;

        /* Free string elements if string array */
        if (arr->type == TYPE_STR && arr->data) {
            for (i = 0; i < arr->size; i++) {
                if (arr->data[i].strval) {
                    free_string(arr->data[i].strval);
                }
            }
        }

        /* Free array data */
        if (arr->data) {
            free(arr->data);
        }

        free(arr);
    }

    g_state->arrlist = NULL;
}
