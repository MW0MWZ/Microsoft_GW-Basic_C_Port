/*
 * variables.c - Variable storage and management
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* K&R C compatible character tests - ctype macros may fail on old systems */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/*
 * Normalize variable name (uppercase, handle type suffix)
 */
static void
normalize_name(dest, src, type)
char *dest;
const char *src;
int *type;
{
    char *d;
    const char *s;
    int len;

    d = dest;
    s = src;
    len = 0;

    /* Copy and convert to uppercase - safe for old systems */
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
                case '$': *type = TYPE_STR; break;
                case '%': *type = TYPE_INT; break;
                case '!': *type = TYPE_SNG; break;
                case '#': *type = TYPE_DBL; break;
            }
            break;
        } else {
            break;
        }
        s++;
    }
    *d = '\0';

    /* Default type if not specified */
    if (*type == 0) {
        *type = TYPE_SNG; /* Default to single precision */
    }
}

/*
 * Find a variable by name, optionally create if not found
 */
var_t *
find_variable(name, create)
const char *name;
int create;
{
    var_t *var;
    var_t *prev;
    char normname[NAMLEN+1];
    int type;

    type = 0;
    normalize_name(normname, name, &type);

    /* Check cache first for O(1) lookup on repeated access */
    if (g_state->lastvar &&
        strcmp(g_state->lastvar->name, normname) == 0 &&
        g_state->lastvar->type == type) {
        return g_state->lastvar;
    }

    /* Search variable list */
    prev = NULL;
    for (var = g_state->varlist; var != NULL; var = var->next) {
        if (strcmp(var->name, normname) == 0 && var->type == type) {
            g_state->lastvar = var;  /* Update cache */
            return var;
        }
        prev = var;
    }

    /* Not found */
    if (!create) {
        return NULL;
    }

    /* Create new variable */
    var = (var_t *)malloc(sizeof(var_t));
    if (!var) {
        error(ERR_OUT_OF_MEM);
        return NULL;
    }

    strcpy(var->name, normname);
    var->type = type;
    var->next = NULL;

    /* Initialize value to zero/empty */
    switch (type) {
        case TYPE_INT:
            var->value.intval = 0;
            break;
        case TYPE_SNG:
            var->value.sngval = 0.0;
            break;
        case TYPE_DBL:
            var->value.dblval = 0.0;
            break;
        case TYPE_STR:
            var->value.strval = alloc_string(0);
            break;
    }

    /* Add to list */
    if (prev) {
        prev->next = var;
    } else {
        g_state->varlist = var;
    }

    g_state->lastvar = var;  /* Update cache */
    return var;
}

/*
 * Set a variable value
 */
void
set_variable(name, val, type)
const char *name;
value_t val;
int type;
{
    var_t *var;
    double dval;

    var = find_variable(name, 1);
    if (!var) {
        return;
    }

    /* Free old string if changing string variable */
    if (var->type == TYPE_STR && var->value.strval) {
        free_string(var->value.strval);
    }

    /* Convert expression value to double first */
    switch (type) {
        case TYPE_INT:
            dval = (double)val.intval;
            break;
        case TYPE_SNG:
            dval = (double)val.sngval;
            break;
        case TYPE_DBL:
            dval = val.dblval;
            break;
        default:
            dval = 0.0;
            break;
    }

    /* Store in variable's native type */
    switch (var->type) {
        case TYPE_INT:
            var->value.intval = (int)dval;
            break;
        case TYPE_SNG:
            var->value.sngval = (float)dval;
            break;
        case TYPE_DBL:
            var->value.dblval = dval;
            break;
        case TYPE_STR:
            if (type == TYPE_STR && val.strval) {
                var->value.strval = copy_string(val.strval);
            } else {
                var->value.strval = alloc_string(0);
            }
            break;
    }
}

/*
 * Get a variable value
 */
value_t
get_variable(name, type)
const char *name;
int *type;
{
    var_t *var;
    value_t result;

    var = find_variable(name, 1);
    if (!var) {
        /* Return zero/empty */
        *type = TYPE_SNG;
        result.sngval = 0.0;
        return result;
    }

    *type = var->type;
    return var->value;
}

/*
 * Clear all variables
 */
void
clear_variables()
{
    var_t *var;
    var_t *next;

    for (var = g_state->varlist; var != NULL; var = next) {
        next = var->next;

        /* Free string data if string variable */
        if (var->type == TYPE_STR && var->value.strval) {
            free_string(var->value.strval);
        }

        free(var);
    }

    g_state->varlist = NULL;
    g_state->lastvar = NULL;  /* Clear cache */
}
