/*
 * eval.c - Expression evaluator
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* K&R C compatible character tests - ctype macros may fail on old systems */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/* Forward declarations */
static value_t expr_or(int *type);
static value_t expr_and(int *type);
static value_t expr_not(int *type);
static value_t expr_compare(int *type);
static value_t expr_add(int *type);
static value_t expr_mult(int *type);
static value_t expr_power(int *type);
static value_t expr_unary(int *type);
static value_t expr_primary(int *type);

/*
 * Get next character from input
 * Mask with 0xFF to ensure unsigned value on K&R C
 */
int
get_next_char()
{
    if (g_state->txtptr && *g_state->txtptr) {
        return *g_state->txtptr++ & 0xFF;
    }
    return '\0';
}

/*
 * Peek at next character without consuming
 * Mask with 0xFF to ensure unsigned value on K&R C
 */
int
peek_char()
{
    if (g_state->txtptr && *g_state->txtptr) {
        return *g_state->txtptr & 0xFF;
    }
    return '\0';
}


/*
 * Skip whitespace
 */
void
skip_spaces()
{
    while (peek_char() == ' ' || peek_char() == '\t') {
        get_next_char();
    }
}

/*
 * Check if next token matches, consume if so
 */
int
match_token(token)
int token;
{
    int c;
    int next;
    unsigned char *saved_txtptr;

    skip_spaces();
    c = peek_char();
    saved_txtptr = g_state->txtptr;  /* Save position for restore on failure */

    /* Note: use & 0xFF00 instead of > 0xFF for 16-bit signed int */
    if (token & 0xFF00) {
        /* Two-byte token - mask comparisons for signed char compatibility */
        if ((c & 0xFF) == ((token >> 8) & 0xFF)) {
            get_next_char();
            next = peek_char();
            if ((next & 0xFF) == (token & 0xFF)) {
                get_next_char();
                return 1;
            }
            g_state->txtptr = saved_txtptr;  /* Restore position */
        }
    } else {
        /* Single-byte token or character */
        if (c == token) {
            get_next_char();
            return 1;
        }
    }

    return 0;
}

/*
 * Parse a number
 */
static value_t
parse_number(type)
int *type;
{
    value_t result;
    char numbuf[80];
    char *p;
    int c;
    int has_dot;
    int has_exp;

    p = numbuf;
    has_dot = 0;
    has_exp = 0;
    *type = TYPE_INT;

    /* Parse number */
    /* Note: explicit range check instead of isdigit() for old K&R C */
    while (1) {
        c = peek_char();

        if (c >= '0' && c <= '9') {
            *p++ = get_next_char();
        } else if (c == '.' && !has_dot && !has_exp) {
            *p++ = get_next_char();
            has_dot = 1;
            *type = TYPE_SNG;
        } else if ((c == 'E' || c == 'e' || c == 'D' || c == 'd') && !has_exp) {
            *p++ = 'E';
            get_next_char();
            has_exp = 1;
            *type = (c == 'D' || c == 'd') ? TYPE_DBL : TYPE_SNG;

            /* Check for sign */
            c = peek_char();
            if (c == '+' || c == '-') {
                *p++ = get_next_char();
            }
        } else {
            break;
        }

        if (p - numbuf >= 79) {
            break;
        }
    }

    *p = '\0';

    /* Check for type suffix */
    c = peek_char();
    if (c == '%') {
        get_next_char();
        *type = TYPE_INT;
    } else if (c == '!') {
        get_next_char();
        *type = TYPE_SNG;
    } else if (c == '#') {
        get_next_char();
        *type = TYPE_DBL;
    }

    /* Convert string to number */
    switch (*type) {
        case TYPE_INT:
            result.intval = atoi(numbuf);
            break;
        case TYPE_SNG:
            result.sngval = (float)atof(numbuf);
            break;
        case TYPE_DBL:
            result.dblval = atof(numbuf);
            break;
    }

    return result;
}

/*
 * Parse a string literal
 */
string_t *
parse_string_literal()
{
    char strbuf[256];
    char *p;
    int c;

    p = strbuf;

    /* Skip opening quote */
    get_next_char();

    /* Read until closing quote or end of line */
    while (1) {
        c = peek_char();
        if (c == '"' || c == '\0' || c == '\n') {
            break;
        }
        *p++ = get_next_char();

        if (p - strbuf >= 255) {
            break;
        }
    }

    *p = '\0';

    /* Skip closing quote if present */
    if (peek_char() == '"') {
        get_next_char();
    }

    return string_from_cstr(strbuf);
}

/*
 * Check if name is a built-in numeric function
 */
static int
is_numeric_function(name)
char *name;
{
    if (strcmp(name, "SQR") == 0) return 1;
    if (strcmp(name, "SIN") == 0) return 1;
    if (strcmp(name, "COS") == 0) return 1;
    if (strcmp(name, "TAN") == 0) return 1;
    if (strcmp(name, "ATN") == 0) return 1;
    if (strcmp(name, "LOG") == 0) return 1;
    if (strcmp(name, "EXP") == 0) return 1;
    if (strcmp(name, "ABS") == 0) return 1;
    if (strcmp(name, "SGN") == 0) return 1;
    if (strcmp(name, "INT") == 0) return 1;
    if (strcmp(name, "RND") == 0) return 1;
    if (strcmp(name, "LEN") == 0) return 1;
    if (strcmp(name, "ASC") == 0) return 1;
    if (strcmp(name, "VAL") == 0) return 1;
    return 0;
}

/*
 * Check if name is a built-in string function
 */
static int
is_string_function(name)
char *name;
{
    if (strcmp(name, "CHR$") == 0) return 1;
    if (strcmp(name, "STR$") == 0) return 1;
    if (strcmp(name, "LEFT$") == 0) return 1;
    if (strcmp(name, "RIGHT$") == 0) return 1;
    if (strcmp(name, "MID$") == 0) return 1;
    return 0;
}

/*
 * Call a numeric function by name
 */
static value_t
call_numeric_function(name, type)
char *name;
int *type;
{
    value_t result;
    double arg;
    string_t *sarg;

    *type = TYPE_DBL;
    result.dblval = 0.0;

    skip_spaces();
    if (peek_char() != '(') {
        syntax_error();
        return result;
    }
    get_next_char(); /* Skip '(' */

    /* Most functions take a single numeric argument */
    if (strcmp(name, "LEN") == 0 || strcmp(name, "ASC") == 0 ||
        strcmp(name, "VAL") == 0) {
        /* String argument */
        sarg = eval_string();
        skip_spaces();
        if (peek_char() == ')') {
            get_next_char();
        }
        if (strcmp(name, "LEN") == 0) {
            result.dblval = fn_len(sarg);
        } else if (strcmp(name, "ASC") == 0) {
            result.dblval = fn_asc(sarg);
        } else if (strcmp(name, "VAL") == 0) {
            result.dblval = fn_val(sarg);
        }
        if (sarg) free_string(sarg);
        return result;
    }

    /* Numeric argument */
    arg = eval_numeric();
    skip_spaces();
    if (peek_char() == ')') {
        get_next_char();
    }

    if (strcmp(name, "SQR") == 0) {
        result.dblval = fn_sqr(arg);
    } else if (strcmp(name, "SIN") == 0) {
        result.dblval = fn_sin(arg);
    } else if (strcmp(name, "COS") == 0) {
        result.dblval = fn_cos(arg);
    } else if (strcmp(name, "TAN") == 0) {
        result.dblval = fn_tan(arg);
    } else if (strcmp(name, "ATN") == 0) {
        result.dblval = fn_atn(arg);
    } else if (strcmp(name, "LOG") == 0) {
        result.dblval = fn_log(arg);
    } else if (strcmp(name, "EXP") == 0) {
        result.dblval = fn_exp(arg);
    } else if (strcmp(name, "ABS") == 0) {
        result.dblval = fn_abs(arg);
    } else if (strcmp(name, "SGN") == 0) {
        result.dblval = fn_sgn(arg);
    } else if (strcmp(name, "INT") == 0) {
        result.dblval = fn_int(arg);
    } else if (strcmp(name, "RND") == 0) {
        result.dblval = fn_rnd(arg);
    }

    return result;
}

/*
 * Call a string function by name
 */
static value_t
call_string_function(name, type)
char *name;
int *type;
{
    value_t result;
    string_t *sarg;
    double narg;
    int n;
    int start;
    int len;

    *type = TYPE_STR;
    result.strval = NULL;

    skip_spaces();
    if (peek_char() != '(') {
        syntax_error();
        return result;
    }
    get_next_char(); /* Skip '(' */

    if (strcmp(name, "CHR$") == 0) {
        narg = eval_numeric();
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        result.strval = fn_chr((int)narg);
    } else if (strcmp(name, "STR$") == 0) {
        narg = eval_numeric();
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        result.strval = fn_str(narg);
    } else if (strcmp(name, "LEFT$") == 0) {
        sarg = eval_string();
        skip_spaces();
        if (peek_char() == ',') get_next_char();
        n = eval_integer();
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        result.strval = fn_left(sarg, n);
        if (sarg) free_string(sarg);
    } else if (strcmp(name, "RIGHT$") == 0) {
        sarg = eval_string();
        skip_spaces();
        if (peek_char() == ',') get_next_char();
        n = eval_integer();
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        result.strval = fn_right(sarg, n);
        if (sarg) free_string(sarg);
    } else if (strcmp(name, "MID$") == 0) {
        sarg = eval_string();
        skip_spaces();
        if (peek_char() == ',') get_next_char();
        start = eval_integer();
        len = 255; /* Default: rest of string */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
            len = eval_integer();
        }
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        result.strval = fn_mid(sarg, start, len);
        if (sarg) free_string(sarg);
    }

    return result;
}

/*
 * Parse a variable or array reference or function call
 */
static value_t
parse_variable(type)
int *type;
{
    char varname[NAMLEN+1];
    char *p;
    int c;
    value_t result;
    int indices[8];
    int nindices;
    value_t *elem;

    p = varname;

    /* Read variable/function name - safe uppercase for old systems */
    while (1) {
        c = peek_char();
        if (IS_ALNUM(c) || c == '.') {
            c = get_next_char();
            if (p - varname < NAMLEN) {
                if (c >= 'a' && c <= 'z') {
                    *p++ = c - 'a' + 'A';
                } else {
                    *p++ = c;
                }
            }
            /* else: skip excess characters (already consumed by get_next_char) */
        } else if (c == '$' || c == '%' || c == '!' || c == '#') {
            if (p - varname < NAMLEN) {
                *p++ = get_next_char();
            } else {
                get_next_char();  /* Consume but don't store */
            }
            break;
        } else {
            break;
        }
    }

    *p = '\0';

    /* Check if it's a built-in function */
    if (is_numeric_function(varname)) {
        return call_numeric_function(varname, type);
    }
    if (is_string_function(varname)) {
        return call_string_function(varname, type);
    }

    /* Check for array subscript */
    skip_spaces();
    if (peek_char() == '(') {
        get_next_char(); /* Skip '(' */

        /* Parse subscripts */
        nindices = 0;
        while (nindices < 8) {
            indices[nindices++] = eval_integer();

            skip_spaces();
            if (peek_char() == ',') {
                get_next_char();
            } else {
                break;
            }
        }

        skip_spaces();
        if (peek_char() == ')') {
            get_next_char();
        } else {
            syntax_error();
        }

        /* Get array element */
        elem = array_element(varname, indices, nindices);
        if (!elem) {
            *type = TYPE_SNG;
            result.sngval = 0.0;
            return result;
        }

        /* Determine type from variable name */
        if (strchr(varname, '$')) {
            *type = TYPE_STR;
            /* Return a copy so caller owns it */
            result.strval = elem->strval ? copy_string(elem->strval) : NULL;
            return result;
        } else {
            *type = TYPE_SNG;
            return *elem;
        }

    } else {
        /* Simple variable */
        result = get_variable(varname, type);

        /* For string variables, return a copy so caller owns it */
        if (*type == TYPE_STR && result.strval) {
            result.strval = copy_string(result.strval);
        }

        return result;
    }
}

/*
 * Handle tokenized function call
 */
static value_t
call_tokenized_function(token, type)
int token;
int *type;
{
    value_t result;
    double arg;
    string_t *sarg;

    *type = TYPE_DBL;
    result.dblval = 0.0;

    skip_spaces();
    if (peek_char() != '(') {
        syntax_error();
        return result;
    }
    get_next_char(); /* Skip '(' */

    /* Functions that take string arguments */
    if (token == TOK_LEN || token == TOK_ASC || token == TOK_VAL) {
        sarg = eval_string();
        skip_spaces();
        if (peek_char() == ')') get_next_char();
        if (token == TOK_LEN) {
            result.dblval = fn_len(sarg);
        } else if (token == TOK_ASC) {
            result.dblval = fn_asc(sarg);
        } else if (token == TOK_VAL) {
            result.dblval = fn_val(sarg);
        }
        if (sarg) free_string(sarg);
        return result;
    }

    /* Functions that take numeric arguments */
    arg = eval_numeric();
    skip_spaces();
    if (peek_char() == ')') get_next_char();

    switch (token) {
        case TOK_SQR: result.dblval = fn_sqr(arg); break;
        case TOK_SIN: result.dblval = fn_sin(arg); break;
        case TOK_COS: result.dblval = fn_cos(arg); break;
        case TOK_TAN: result.dblval = fn_tan(arg); break;
        case TOK_ATN: result.dblval = fn_atn(arg); break;
        case TOK_LOG: result.dblval = fn_log(arg); break;
        case TOK_EXP: result.dblval = fn_exp(arg); break;
        case TOK_ABS: result.dblval = fn_abs(arg); break;
        case TOK_SGN: result.dblval = fn_sgn(arg); break;
        case TOK_INT: result.dblval = fn_int(arg); break;
        case TOK_RND: result.dblval = fn_rnd(arg); break;
        case TOK_FRE: result.dblval = fn_fre(arg); break;
        default:
            result.dblval = 0.0;
            break;
    }

    return result;
}

/*
 * Handle tokenized string function call
 */
static value_t
call_tokenized_str_function(token, type)
int token;
int *type;
{
    value_t result;
    string_t *sarg;
    double narg;
    int n;
    int start;
    int len;

    *type = TYPE_STR;
    result.strval = NULL;

    skip_spaces();
    if (peek_char() != '(') {
        syntax_error();
        return result;
    }
    get_next_char(); /* Skip '(' */

    switch (token) {
        case TOK_CHR:
            narg = eval_numeric();
            skip_spaces();
            if (peek_char() == ')') get_next_char();
            result.strval = fn_chr((int)narg);
            break;

        case TOK_STR:
            narg = eval_numeric();
            skip_spaces();
            if (peek_char() == ')') get_next_char();
            result.strval = fn_str(narg);
            break;

        case TOK_LEFT:
            sarg = eval_string();
            skip_spaces();
            if (peek_char() == ',') get_next_char();
            n = eval_integer();
            skip_spaces();
            if (peek_char() == ')') get_next_char();
            result.strval = fn_left(sarg, n);
            if (sarg) free_string(sarg);
            break;

        case TOK_RIGHT:
            sarg = eval_string();
            skip_spaces();
            if (peek_char() == ',') get_next_char();
            n = eval_integer();
            skip_spaces();
            if (peek_char() == ')') get_next_char();
            result.strval = fn_right(sarg, n);
            if (sarg) free_string(sarg);
            break;

        case TOK_MID:
            sarg = eval_string();
            skip_spaces();
            if (peek_char() == ',') get_next_char();
            start = eval_integer();
            len = 255;
            skip_spaces();
            if (peek_char() == ',') {
                get_next_char();
                len = eval_integer();
            }
            skip_spaces();
            if (peek_char() == ')') get_next_char();
            result.strval = fn_mid(sarg, start, len);
            if (sarg) free_string(sarg);
            break;

        default:
            break;
    }

    return result;
}

/*
 * Primary expression: number, string, variable, function, (expr)
 */
static value_t
expr_primary(type)
int *type;
{
    value_t result;
    int c;
    int token;
    string_t *str;

    skip_spaces();
    c = peek_char();

    /* Number */
    /* Note: explicit range check instead of isdigit() for old K&R C */
    /* Mask txtptr[1] with 0xFF for signed char compatibility */
    if ((c >= '0' && c <= '9') || (c == '.' && (g_state->txtptr[1] & 0xFF) >= '0' && (g_state->txtptr[1] & 0xFF) <= '9')) {
        return parse_number(type);
    }

    /* String literal */
    if (c == '"') {
        *type = TYPE_STR;
        str = parse_string_literal();
        result.strval = str;
        return result;
    }

    /* Parenthesized expression */
    if (c == '(') {
        get_next_char();
        result = eval_expr(type);
        skip_spaces();
        if (peek_char() == ')') {
            get_next_char();
        } else {
            syntax_error();
        }
        return result;
    }

    /* Variable or function (text form) */
    if (IS_ALPHA(c)) {
        return parse_variable(type);
    }

    /* Tokenized function (two-byte token starting with 0xFF) */
    /* Note: use & 0xFF for K&R C where char may be signed */
    if ((c & 0xFF) == 0xFF) {
        get_next_char(); /* Consume 0xFF */
        token = (0xFF << 8) | get_next_char();

        /* Check if it's a numeric function */
        if (token == TOK_SQR || token == TOK_SIN || token == TOK_COS ||
            token == TOK_TAN || token == TOK_ATN || token == TOK_LOG ||
            token == TOK_EXP || token == TOK_ABS || token == TOK_SGN ||
            token == TOK_INT || token == TOK_RND || token == TOK_LEN ||
            token == TOK_ASC || token == TOK_VAL || token == TOK_FRE) {
            return call_tokenized_function(token, type);
        }

        /* Check if it's a string function */
        if (token == TOK_CHR || token == TOK_STR || token == TOK_LEFT ||
            token == TOK_RIGHT || token == TOK_MID) {
            return call_tokenized_str_function(token, type);
        }

        /* Unknown 0xFF token - don't put it back (would cause infinite loop) */
        /* Just consume it and return zero */
        *type = TYPE_SNG;
        result.sngval = 0.0;
        return result;
    }

    /* Default: zero */
    *type = TYPE_SNG;
    result.sngval = 0.0;
    return result;
}

/*
 * Unary operators: +, -, NOT
 */
static value_t
expr_unary(type)
int *type;
{
    value_t result;

    skip_spaces();

    /* Unary plus */
    if (match_token(TOK_PLUS) || peek_char() == '+') {
        if (peek_char() == '+') {
            get_next_char();
        }
        return expr_unary(type);
    }

    /* Unary minus */
    if (match_token(TOK_MINUS) || peek_char() == '-') {
        if (peek_char() == '-') {
            get_next_char();
        }
        result = expr_unary(type);
        switch (*type) {
            case TYPE_INT:
                result.intval = -result.intval;
                break;
            case TYPE_SNG:
                result.sngval = -result.sngval;
                break;
            case TYPE_DBL:
                result.dblval = -result.dblval;
                break;
            default:
                syntax_error();
        }
        return result;
    }

    /* NOT operator */
    if (match_token(TOK_NOT)) {
        result = expr_unary(type);
        if (*type == TYPE_INT) {
            result.intval = ~result.intval;
        } else {
            result.intval = ~((int)result.sngval);
            *type = TYPE_INT;
        }
        return result;
    }

    return expr_primary(type);
}

/*
 * Power operator: ^
 */
static value_t
expr_power(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;
    double base;
    double exponent;

    left = expr_unary(type);

    while (match_token(TOK_POWER) || peek_char() == '^') {
        if (peek_char() == '^') {
            get_next_char();
        }
        right = expr_unary(&rtype);

        /* Convert to double for power operation */
        switch (*type) {
            case TYPE_INT: base = (double)left.intval; break;
            case TYPE_SNG: base = (double)left.sngval; break;
            case TYPE_DBL: base = left.dblval; break;
            default: base = 0.0;
        }

        switch (rtype) {
            case TYPE_INT: exponent = (double)right.intval; break;
            case TYPE_SNG: exponent = (double)right.sngval; break;
            case TYPE_DBL: exponent = right.dblval; break;
            default: exponent = 0.0;
        }

        left.dblval = pow(base, exponent);
        *type = TYPE_DBL;
    }

    return left;
}

/*
 * Multiplicative operators: *, /, \, MOD
 */
static value_t
expr_mult(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;
    int op;

    left = expr_power(type);

    while (1) {
        skip_spaces();

        if (match_token(TOK_MULT) || peek_char() == '*') {
            op = '*';
            if (peek_char() == '*') {
                get_next_char();
            }
        } else if (match_token(TOK_DIV) || peek_char() == '/') {
            op = '/';
            if (peek_char() == '/') {
                get_next_char();
            }
        } else if (match_token(TOK_IDIV) || peek_char() == '\\') {
            op = '\\';
            if (peek_char() == '\\') {
                get_next_char();
            }
        } else if (match_token(TOK_MOD)) {
            op = '%';
        } else {
            break;
        }

        right = expr_power(&rtype);

        /* Perform operation */
        if (op == '*') {
            if (*type == TYPE_INT && rtype == TYPE_INT) {
                left.intval *= right.intval;
            } else {
                double lval = (*type == TYPE_INT) ? (double)left.intval :
                              (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
                double rval = (rtype == TYPE_INT) ? (double)right.intval :
                              (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
                left.dblval = lval * rval;
                *type = TYPE_DBL;
            }
        } else if (op == '/') {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            if (rval == 0.0) {
                error(ERR_DIV_ZERO);
            }
            left.dblval = lval / rval;
            *type = TYPE_DBL;
        } else if (op == '\\') {
            int lval = (*type == TYPE_INT) ? left.intval : (int)left.sngval;
            int rval = (rtype == TYPE_INT) ? right.intval : (int)right.sngval;
            if (rval == 0) {
                error(ERR_DIV_ZERO);
            }
            left.intval = lval / rval;
            *type = TYPE_INT;
        } else if (op == '%') {
            int lval = (*type == TYPE_INT) ? left.intval : (int)left.sngval;
            int rval = (rtype == TYPE_INT) ? right.intval : (int)right.sngval;
            if (rval == 0) {
                error(ERR_DIV_ZERO);
            }
            left.intval = lval % rval;
            *type = TYPE_INT;
        }
    }

    return left;
}

/*
 * Additive operators: +, -
 */
static value_t
expr_add(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;
    int op;
    string_t *result;

    left = expr_mult(type);

    while (1) {
        skip_spaces();

        if (match_token(TOK_PLUS) || peek_char() == '+') {
            op = '+';
            if (peek_char() == '+') {
                get_next_char();
            }
        } else if (match_token(TOK_MINUS) || peek_char() == '-') {
            op = '-';
            if (peek_char() == '-') {
                get_next_char();
            }
        } else {
            break;
        }

        right = expr_mult(&rtype);

        /* String concatenation */
        if (*type == TYPE_STR && rtype == TYPE_STR && op == '+') {
            result = concat_strings(left.strval, right.strval);
            /* Free original strings (they are temporary copies) */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
            left.strval = result;
        } else if (*type == TYPE_INT && rtype == TYPE_INT) {
            if (op == '+') {
                left.intval += right.intval;
            } else {
                left.intval -= right.intval;
            }
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            if (op == '+') {
                left.dblval = lval + rval;
            } else {
                left.dblval = lval - rval;
            }
            *type = TYPE_DBL;
        }
    }

    return left;
}

/*
 * Comparison operators: =, <>, <, >, <=, >=
 */
static value_t
expr_compare(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;
    int cmp;
    int result_val;

    left = expr_add(type);

    skip_spaces();

    if (match_token(TOK_EQ) || peek_char() == '=') {
        if (peek_char() == '=') {
            get_next_char();
        }
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp == 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval == rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;

    } else if (match_token(TOK_NE)) {
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp != 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval != rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;

    } else if (match_token(TOK_LT) || peek_char() == '<') {
        if (peek_char() == '<') {
            get_next_char();
        }
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp < 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval < rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;

    } else if (match_token(TOK_GT) || peek_char() == '>') {
        if (peek_char() == '>') {
            get_next_char();
        }
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp > 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval > rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;

    } else if (match_token(TOK_LE)) {
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp <= 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval <= rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;

    } else if (match_token(TOK_GE)) {
        right = expr_add(&rtype);

        if (*type == TYPE_STR && rtype == TYPE_STR) {
            cmp = compare_strings(left.strval, right.strval);
            result_val = (cmp >= 0) ? -1 : 0;
            /* Free temporary strings after comparison */
            if (left.strval) free_string(left.strval);
            if (right.strval) free_string(right.strval);
        } else {
            double lval = (*type == TYPE_INT) ? (double)left.intval :
                          (*type == TYPE_SNG) ? (double)left.sngval : left.dblval;
            double rval = (rtype == TYPE_INT) ? (double)right.intval :
                          (rtype == TYPE_SNG) ? (double)right.sngval : right.dblval;
            result_val = (lval >= rval) ? -1 : 0;
        }
        left.intval = result_val;
        *type = TYPE_INT;
    }

    return left;
}

/*
 * NOT operator (handled in unary)
 */
static value_t
expr_not(type)
int *type;
{
    return expr_compare(type);
}

/*
 * AND operator
 */
static value_t
expr_and(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;

    left = expr_not(type);

    while (match_token(TOK_AND)) {
        right = expr_not(&rtype);

        if (*type == TYPE_INT && rtype == TYPE_INT) {
            left.intval &= right.intval;
        } else {
            int lval = (*type == TYPE_INT) ? left.intval : (int)left.sngval;
            int rval = (rtype == TYPE_INT) ? right.intval : (int)right.sngval;
            left.intval = lval & rval;
            *type = TYPE_INT;
        }
    }

    return left;
}

/*
 * OR, XOR operators
 */
static value_t
expr_or(type)
int *type;
{
    value_t left;
    value_t right;
    int rtype;
    int op;

    left = expr_and(type);

    while (1) {
        if (match_token(TOK_OR)) {
            op = '|';
        } else if (match_token(TOK_XOR)) {
            op = '^';
        } else {
            break;
        }

        right = expr_and(&rtype);

        if (op == '|') {
            if (*type == TYPE_INT && rtype == TYPE_INT) {
                left.intval |= right.intval;
            } else {
                int lval = (*type == TYPE_INT) ? left.intval : (int)left.sngval;
                int rval = (rtype == TYPE_INT) ? right.intval : (int)right.sngval;
                left.intval = lval | rval;
                *type = TYPE_INT;
            }
        } else {
            if (*type == TYPE_INT && rtype == TYPE_INT) {
                left.intval ^= right.intval;
            } else {
                int lval = (*type == TYPE_INT) ? left.intval : (int)left.sngval;
                int rval = (rtype == TYPE_INT) ? right.intval : (int)right.sngval;
                left.intval = lval ^ rval;
                *type = TYPE_INT;
            }
        }
    }

    return left;
}

/*
 * Main expression evaluator
 */
value_t
eval_expr(type)
int *type;
{
    return expr_or(type);
}

/*
 * Evaluate numeric expression
 */
double
eval_numeric()
{
    value_t val;
    int type;

    val = eval_expr(&type);

    switch (type) {
        case TYPE_INT:
            return (double)val.intval;
        case TYPE_SNG:
            return (double)val.sngval;
        case TYPE_DBL:
            return val.dblval;
        case TYPE_STR:
            /* Free string if expression returned one - shouldn't happen normally */
            if (val.strval) free_string(val.strval);
            return 0.0;
        default:
            return 0.0;
    }
}

/*
 * Evaluate string expression
 */
string_t *
eval_string()
{
    value_t val;
    int type;

    val = eval_expr(&type);

    if (type != TYPE_STR) {
        syntax_error();
        return alloc_string(0);
    }

    return val.strval;
}

/*
 * Evaluate integer expression
 */
int
eval_integer()
{
    value_t val;
    int type;

    val = eval_expr(&type);

    switch (type) {
        case TYPE_INT:
            return val.intval;
        case TYPE_SNG:
            return (int)val.sngval;
        case TYPE_DBL:
            return (int)val.dblval;
        case TYPE_STR:
            /* Free string if expression returned one - shouldn't happen normally */
            if (val.strval) free_string(val.strval);
            return 0;
        default:
            return 0;
    }
}
