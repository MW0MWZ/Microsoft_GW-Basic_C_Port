/*
 * statements.c - BASIC statement implementations
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* K&R C compatible character tests - ctype macros may fail on old systems */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))

/* For sleep/delay functions */
#if defined(__211BSD__) || defined(pdp11) || !defined(__STDC__)
/* 2.11 BSD - use select() for sub-second delays */
#include <sys/types.h>
#include <sys/time.h>
extern int select();
#else
/* Modern systems - use usleep() */
#include <unistd.h>
#endif

/*
 * PRINT statement
 */
void
do_print()
{
    value_t val;
    int type;
    int col;
    int newline;
    int tabpos;

    col = 0;
    newline = 1;

    while (1) {
        skip_spaces();

        /* Check for end of statement */
        /* Also stop on ELSE token */
        if (peek_char() == '\0' || peek_char() == ':' ||
            peek_char() == TOK_ELSE) {
            break;
        }

        /* Semicolon - no newline, no spacing */
        if (peek_char() == ';') {
            get_next_char();
            newline = 0;
            continue;
        }

        /* Comma - tab to next column */
        if (peek_char() == ',') {
            get_next_char();
            col = (col / 14 + 1) * 14;
            while (col % 14 != 0) {
                putchar(' ');
                col++;
            }
            newline = 0;
            continue;
        }

        /* TAB function - move to specified column */
        if (match_token(TOK_TAB)) {
            if (peek_char() == '(') {
                get_next_char();  /* skip ( */
                tabpos = eval_integer();
                if (peek_char() == ')') {
                    get_next_char();  /* skip ) */
                }
            } else {
                tabpos = eval_integer();
            }
            /* Move to column position */
            while (col < tabpos) {
                putchar(' ');
                col++;
            }
            newline = 0;
            continue;
        }

        /* Evaluate and print expression */
        val = eval_expr(&type);

        switch (type) {
            case TYPE_INT:
                printf("%d", val.intval);
                col += 6; /* Approximate */
                break;

            case TYPE_SNG:
                printf("%g", val.sngval);
                col += 10; /* Approximate */
                break;

            case TYPE_DBL:
                printf("%g", val.dblval);
                col += 16; /* Approximate */
                break;

            case TYPE_STR:
                if (val.strval && val.strval->ptr) {
                    printf("%.*s", val.strval->len, val.strval->ptr);
                    col += val.strval->len;
                }
                /* Free temporary string (eval_expr returns owned copy) */
                if (val.strval) {
                    free_string(val.strval);
                }
                break;
        }

        newline = 1;
    }

    if (newline) {
        putchar('\n');
    }
}

/*
 * INPUT statement
 */
void
do_input()
{
    char prompt[256];
    char *p;
    char varname[NAMLEN+1];
    value_t val;
    int type;
    int has_prompt;
    string_t *str;
    char *line;

    has_prompt = 0;
    prompt[0] = '\0';

    skip_spaces();

    /* Check for string literal prompt */
    if (peek_char() == '"') {
        str = parse_string_literal();
        if (str && str->ptr) {
            strncpy(prompt, str->ptr, 255);
            prompt[255] = '\0';
        }
        free_string(str);
        has_prompt = 1;

        skip_spaces();
        if (peek_char() == ';') {
            get_next_char();
        }
    }

    /* Print prompt */
    if (has_prompt) {
        printf("%s", prompt);
    } else {
        printf("? ");
    }
    fflush(stdout);

    /* Read input */
    if (fgets(g_state->inputbuf, BUFLEN, stdin) == NULL) {
        return;
    }

    /* Parse variable name(s) and assign values */
    line = g_state->inputbuf;

    while (*line) {
        /* Skip whitespace */
        while (*line == ' ' || *line == '\t') {
            line++;
        }

        /* Get variable name from original statement (with length limit) */
        skip_spaces();
        p = varname;
        while (IS_ALNUM(peek_char()) || peek_char() == '.' ||
               peek_char() == '$' || peek_char() == '%' ||
               peek_char() == '!' || peek_char() == '#') {
            if (p - varname < NAMLEN) {
                *p++ = get_next_char();
            } else {
                get_next_char();  /* Skip excess characters */
            }
        }
        *p = '\0';

        if (varname[0] == '\0') {
            break;
        }

        /* Determine type from variable name */
        type = TYPE_SNG;
        if (strchr(varname, '$')) {
            type = TYPE_STR;
        } else if (strchr(varname, '%')) {
            type = TYPE_INT;
        } else if (strchr(varname, '#')) {
            type = TYPE_DBL;
        }

        /* Parse value from input line */
        if (type == TYPE_STR) {
            /* String input */
            p = line;
            while (*p && *p != ',' && *p != '\n') {
                p++;
            }
            *p = '\0';
            val.strval = string_from_cstr(line);
            line = p + 1;
        } else {
            /* Numeric input */
            val.dblval = atof(line);
            val.sngval = (float)val.dblval;
            val.intval = (int)val.dblval;

            /* Skip to next comma or end */
            while (*line && *line != ',' && *line != '\n') {
                line++;
            }
            if (*line == ',') {
                line++;
            }
        }

        /* Assign to variable */
        set_variable(varname, val, type);

        /* Free temporary string after assignment (set_variable copies it) */
        if (type == TYPE_STR && val.strval) {
            free_string(val.strval);
        }

        /* Check for more variables */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            break;
        }
    }
}

/*
 * LET statement (assignment)
 */
void
do_let()
{
    char varname[NAMLEN+1];
    char *p;
    value_t val;
    int type;
    int indices[8];
    int nindices;
    value_t *elem;

    /* Parse variable name (with length limit) */
    skip_spaces();
    p = varname;
    while (IS_ALNUM(peek_char()) || peek_char() == '.' ||
           peek_char() == '$' || peek_char() == '%' ||
           peek_char() == '!' || peek_char() == '#') {
        if (p - varname < NAMLEN) {
            *p++ = get_next_char();
        } else {
            get_next_char();  /* Skip excess characters */
        }
    }
    *p = '\0';

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

        /* Skip '=' */
        skip_spaces();
        if (peek_char() == '=' || match_token(TOK_EQ)) {
            if (peek_char() == '=') {
                get_next_char();
            }
        } else {
            syntax_error();
        }

        /* Evaluate expression */
        val = eval_expr(&type);

        /* Assign to array element */
        elem = array_element(varname, indices, nindices);
        if (elem) {
            /* Free old string in array element before overwriting */
            if (type == TYPE_STR && elem->strval) {
                free_string(elem->strval);
            }
            *elem = val;
            /* Note: array now owns val.strval, don't free it */
        } else {
            /* array_element returned NULL (error) - free the string if needed */
            if (type == TYPE_STR && val.strval) {
                free_string(val.strval);
            }
        }

    } else {
        /* Simple variable assignment */

        /* Skip '=' */
        skip_spaces();
        if (peek_char() == '=' || match_token(TOK_EQ)) {
            if (peek_char() == '=') {
                get_next_char();
            }
        } else {
            syntax_error();
        }

        /* Evaluate expression */
        val = eval_expr(&type);

        /* Assign to variable */
        set_variable(varname, val, type);

        /* Free temporary string after assignment (set_variable copies it) */
        if (type == TYPE_STR && val.strval) {
            free_string(val.strval);
        }
    }
}

/*
 * Skip one token or character properly
 * Handles two-byte tokens (0xFF prefix), single-byte tokens (0x80-0xFE),
 * and regular characters
 */
static void
skip_one_token()
{
    int c;

    c = peek_char();
    if (c == '\0') {
        return;
    }

    if (c == '"') {
        /* Skip string literal */
        get_next_char();
        while (peek_char() != '\0' && peek_char() != '"') {
            get_next_char();
        }
        if (peek_char() == '"') {
            get_next_char();
        }
    } else if ((c & 0xFF) == 0xFF) {
        /* Two-byte token */
        /* Note: use & 0xFF for K&R C where char may be signed */
        get_next_char();
        get_next_char();
    } else if (c & 0x80) {
        /* Single-byte token (high bit set) */
        get_next_char();
    } else {
        /* Regular character */
        get_next_char();
    }
}

/*
 * IF statement
 */
void
do_if()
{
    double condition;
    int target_line;
    line_t *line;
    int c;

    /* Evaluate condition */
    condition = eval_numeric();

    /* Skip THEN keyword if present */
    skip_spaces();
    match_token(TOK_THEN);

    skip_spaces();

    /* Check if condition is true (non-zero) */
    if (condition != 0.0) {
        /* Condition is true, execute THEN part */

        /* Check if THEN is followed by a line number (GOTO) */
        /* Note: explicit range check instead of isdigit() for old K&R C */
        if (peek_char() >= '0' && peek_char() <= '9') {
            target_line = eval_integer();
            line = find_line(target_line);
            if (!line) {
                error(ERR_UNDEF_LINE);
                return;
            }
            g_state->curlin = target_line;
            g_state->txtptr = line->text;
            g_state->curline_ptr = line;  /* Update for fast advance */
            return;
        }

        /* Execute statements until ELSE or end of line */
        while (peek_char() != '\0' && g_state->running) {
            /* Check for ELSE (single-byte token 0xA2) - if found, skip rest */
            c = peek_char();
            if (c == TOK_ELSE) {
                skip_to_eol();
                return;
            }

            execute_statement();
            skip_spaces();

            /* Check what's next */
            c = peek_char();

            /* If ELSE, skip rest of line and return */
            if (c == TOK_ELSE) {
                skip_to_eol();
                return;
            }

            /* If colon, more statements on THEN branch */
            if (c == ':') {
                get_next_char();
                continue;
            }

            /* End of line or something else - done */
            break;
        }
        /* Done with THEN part */

    } else {
        /* Condition is false, skip to ELSE or end of line */

        /* Scan for ELSE token (single-byte 0xA2), skipping tokens properly */
        while (peek_char() != '\0') {
            c = peek_char();

            /* Check for ELSE (single-byte token 0xA2) */
            if (c == TOK_ELSE) {
                get_next_char(); /* consume ELSE token */
                skip_spaces();

                /* Check if ELSE is followed by line number */
                /* Note: explicit range check instead of isdigit() for old K&R C */
                if (peek_char() >= '0' && peek_char() <= '9') {
                    target_line = eval_integer();
                    line = find_line(target_line);
                    if (!line) {
                        error(ERR_UNDEF_LINE);
                        return;
                    }
                    g_state->curlin = target_line;
                    g_state->txtptr = line->text;
                    g_state->curline_ptr = line;  /* Update for fast advance */
                    return;
                }

                /* Execute ELSE part */
                while (peek_char() != '\0' && g_state->running) {
                    execute_statement();
                    skip_spaces();
                    if (peek_char() == ':') {
                        get_next_char();
                    } else {
                        break;
                    }
                }
                return;
            }

            /* Skip to next token */
            skip_one_token();
        }
        /* No ELSE found, just return (condition was false, nothing to do) */
    }
}

/*
 * GOTO statement
 */
void
do_goto()
{
    int target;
    line_t *line;

    /* Get target line number */
    target = eval_integer();

    /* Find target line */
    line = find_line(target);
    if (!line) {
        error(ERR_UNDEF_LINE);
        return;
    }

    /* Jump to target */
    g_state->curlin = target;
    g_state->txtptr = line->text;
    g_state->curline_ptr = line;  /* Update line pointer for fast advance */
}

/*
 * GOSUB statement
 */
void
do_gosub()
{
    int target;
    line_t *line;

    /* Check stack space */
    if (g_state->gosubsp >= STACK_SIZE) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Get target line number */
    target = eval_integer();

    /* Find target line */
    line = find_line(target);
    if (!line) {
        error(ERR_UNDEF_LINE);
        return;
    }

    /* Push return address */
    g_state->gosubstack[g_state->gosubsp].linenum = g_state->curlin;
    g_state->gosubstack[g_state->gosubsp].text = g_state->txtptr;
    g_state->gosubsp++;

    /* Jump to subroutine */
    g_state->curlin = target;
    g_state->txtptr = line->text;
    g_state->curline_ptr = line;  /* Update line pointer for fast advance */
}

/*
 * RETURN statement
 */
void
do_return()
{
    line_t *line;

    /* Check if we have a return address */
    if (g_state->gosubsp == 0) {
        error(ERR_RETURN);
        return;
    }

    /* Pop return address */
    g_state->gosubsp--;
    g_state->curlin = g_state->gosubstack[g_state->gosubsp].linenum;
    g_state->txtptr = g_state->gosubstack[g_state->gosubsp].text;

    /* Update line pointer for fast advance */
    line = find_line(g_state->curlin);
    if (line) {
        g_state->curline_ptr = line;
    }
}

/*
 * FOR statement
 */
void
do_for()
{
    char varname[NAMLEN+1];
    char *p;
    double start_val;
    double limit;
    double step;
    value_t val;

    /* Parse variable name (with length limit) */
    skip_spaces();
    p = varname;
    while (IS_ALNUM(peek_char()) || peek_char() == '.') {
        if (p - varname < NAMLEN) {
            *p++ = get_next_char();
        } else {
            get_next_char();  /* Skip excess characters */
        }
    }
    *p = '\0';

    /* Skip '=' */
    skip_spaces();
    if (peek_char() == '=' || match_token(TOK_EQ)) {
        if (peek_char() == '=') {
            get_next_char();
        }
    } else {
        syntax_error();
    }

    /* Get start value */
    start_val = eval_numeric();

    /* Set loop variable */
    val.dblval = start_val;
    set_variable(varname, val, TYPE_DBL);

    /* Skip TO keyword */
    skip_spaces();
    if (!match_token(TOK_TO)) {
        syntax_error();
    }

    /* Get limit */
    limit = eval_numeric();

    /* Check for STEP */
    step = 1.0;
    skip_spaces();
    if (match_token(TOK_STEP)) {
        step = eval_numeric();
    }

    /* Check stack space */
    if (g_state->forsp >= STACK_SIZE) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Push FOR loop info */
    g_state->forstack[g_state->forsp].linenum = g_state->curlin;
    g_state->forstack[g_state->forsp].text = g_state->txtptr;
    strcpy(g_state->forstack[g_state->forsp].varname, varname);
    g_state->forstack[g_state->forsp].limit = limit;
    g_state->forstack[g_state->forsp].step = step;
    g_state->forsp++;
}

/*
 * NEXT statement
 */
void
do_next()
{
    char varname[NAMLEN+1];
    char *p;
    double current;
    double limit;
    double step;
    value_t val;
    int type;
    int done;
    line_t *line;

    /* Parse variable name (optional, with length limit) */
    skip_spaces();
    p = varname;
    if (IS_ALPHA(peek_char())) {
        while (IS_ALNUM(peek_char()) || peek_char() == '.') {
            if (p - varname < NAMLEN) {
                *p++ = get_next_char();
            } else {
                get_next_char();  /* Skip excess characters */
            }
        }
        *p = '\0';
    } else {
        /* Use variable from most recent FOR */
        if (g_state->forsp == 0) {
            error(ERR_NEXT_NO_FOR);
            return;
        }
        strcpy(varname, g_state->forstack[g_state->forsp-1].varname);
    }

    /* Check if we have a matching FOR */
    if (g_state->forsp == 0) {
        error(ERR_NEXT_NO_FOR);
        return;
    }

    /* Get loop info */
    limit = g_state->forstack[g_state->forsp-1].limit;
    step = g_state->forstack[g_state->forsp-1].step;

    /* Get current value and increment */
    val = get_variable(varname, &type);
    switch (type) {
        case TYPE_INT: current = (double)val.intval; break;
        case TYPE_SNG: current = (double)val.sngval; break;
        case TYPE_DBL: current = val.dblval; break;
        default: current = 0.0; break;
    }
    current += step;

    /* Check if loop is done */
    done = 0;
    if (step >= 0) {
        if (current > limit) {
            done = 1;
        }
    } else {
        if (current < limit) {
            done = 1;
        }
    }

    if (done) {
        /* Exit loop */
        g_state->forsp--;
    } else {
        /* Continue loop */
        val.dblval = current;
        set_variable(varname, val, TYPE_DBL);

        /* Jump back to FOR */
        g_state->curlin = g_state->forstack[g_state->forsp-1].linenum;
        g_state->txtptr = g_state->forstack[g_state->forsp-1].text;

        /* Update line pointer for fast advance */
        line = find_line(g_state->curlin);
        if (line) {
            g_state->curline_ptr = line;
        }
    }
}

/*
 * WHILE statement
 */
void
do_while()
{
    double condition;
    int depth;

    /* Check stack space */
    if (g_state->whilesp >= STACK_SIZE) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Evaluate condition */
    condition = eval_numeric();

    /* Push WHILE loop info */
    g_state->whilestack[g_state->whilesp].linenum = g_state->curlin;
    g_state->whilestack[g_state->whilesp].text = g_state->txtptr;
    g_state->whilesp++;

    /* If condition is false, skip to WEND */
    if (condition == 0.0) {
        /* Find matching WEND */
        depth = 1;
        while (depth > 0) {
            skip_to_eol();
            /* This is simplified - should scan for WHILE/WEND tokens */
            /* For now, just exit the loop */
            g_state->whilesp--;
            break;
        }
    }
}

/*
 * WEND statement
 */
void
do_wend()
{
    line_t *line;

    /* Check if we have a matching WHILE */
    if (g_state->whilesp == 0) {
        syntax_error();
        return;
    }

    /* Jump back to WHILE */
    g_state->whilesp--;
    g_state->curlin = g_state->whilestack[g_state->whilesp].linenum;
    g_state->txtptr = g_state->whilestack[g_state->whilesp].text;

    /* Update line pointer for fast advance */
    line = find_line(g_state->curlin);
    if (line) {
        g_state->curline_ptr = line;
    }
}

/*
 * DIM statement
 */
void
do_dim()
{
    char arrname[NAMLEN+1];
    char *p;
    int dims[8];
    int ndims;
    int type;

    while (1) {
        /* Parse array name (with length limit) */
        skip_spaces();
        p = arrname;
        type = TYPE_SNG;

        while (IS_ALNUM(peek_char()) || peek_char() == '.') {
            if (p - arrname < NAMLEN) {
                *p++ = get_next_char();
            } else {
                get_next_char();  /* Skip excess characters */
            }
        }

        /* Check for type suffix */
        if (peek_char() == '$' || peek_char() == '%' ||
            peek_char() == '!' || peek_char() == '#') {
            if (p - arrname < NAMLEN) {
                *p++ = peek_char();
            }
            if (peek_char() == '$') {
                type = TYPE_STR;
            } else if (peek_char() == '%') {
                type = TYPE_INT;
            } else if (peek_char() == '#') {
                type = TYPE_DBL;
            }
            get_next_char();
        }

        *p = '\0';

        /* Parse dimensions */
        skip_spaces();
        if (peek_char() != '(') {
            syntax_error();
            return;
        }
        get_next_char();

        ndims = 0;
        while (ndims < 8) {
            dims[ndims++] = eval_integer() + 1; /* BASIC uses 0-based, add 1 for size */

            skip_spaces();
            if (peek_char() == ',') {
                get_next_char();
            } else {
                break;
            }
        }

        skip_spaces();
        if (peek_char() != ')') {
            syntax_error();
            return;
        }
        get_next_char();

        /* Dimension the array */
        dimension_array(arrname, dims, ndims, type);

        /* Check for more arrays */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            break;
        }
    }
}

/*
 * DATA statement (just skip it during execution)
 */
void
do_data()
{
    /* DATA statements are only used by READ, skip during execution */
    skip_to_eol();
}

/*
 * READ statement
 */
void
do_read()
{
    /* READ statement - simplified version */
    /* This would need to track DATA statements and current position */
    error(ERR_OUT_OF_DATA);
}

/*
 * RESTORE statement
 */
void
do_restore()
{
    /* Reset DATA pointer to beginning */
    g_state->datlin = 0;
    g_state->datptr = NULL;
}

/*
 * END statement
 */
void
do_end()
{
    g_state->running = 0;
    g_state->curlin = 0;
    printf("\n");
}

/*
 * STOP statement
 */
void
do_stop()
{
    g_state->running = 0;
    printf("Break in %d\n", g_state->curlin);
}

/*
 * CONT statement
 */
void
do_cont()
{
    /* Continue execution after STOP */
    if (g_state->curlin == 0) {
        error(ERR_CANT_CONT);
        return;
    }
    g_state->running = 1;
}

/*
 * NEW statement
 */
void
do_new()
{
    new_program();
}

/*
 * LIST statement
 */
void
do_list()
{
    int start;
    int end;

    start = 0;
    end = MAXLIN;

    skip_spaces();
    /* Note: explicit range check instead of isdigit() for old K&R C */
    if (peek_char() >= '0' && peek_char() <= '9') {
        start = eval_integer();
        end = start;

        skip_spaces();
        if (peek_char() == '-') {
            get_next_char();
            skip_spaces();
            if (peek_char() >= '0' && peek_char() <= '9') {
                end = eval_integer();
            } else {
                end = MAXLIN;
            }
        }
    }

    list_program(start, end);
}

/*
 * RUN statement
 */
void
do_run()
{
    int startline;

    startline = 0;

    skip_spaces();
    /* Note: explicit range check instead of isdigit() for old K&R C */
    if (peek_char() >= '0' && peek_char() <= '9') {
        startline = eval_integer();
    }

    /* Clear variables and run */
    clear_variables();
    clear_arrays();
    g_state->forsp = 0;
    g_state->gosubsp = 0;
    g_state->whilesp = 0;

    run_program(startline);
}

/*
 * LOAD statement
 */
void
do_load()
{
    string_t *filename;
    char *fname;

    filename = eval_string();
    if (!filename) {
        return;
    }

    fname = string_to_cstr(filename);
    if (!fname) {
        free_string(filename);
        return;
    }

    if (load_file(fname) != 0) {
        error(ERR_FILE_NOTFND);
    }

    free(fname);
    free_string(filename);
}

/*
 * SAVE statement
 */
void
do_save()
{
    string_t *filename;
    char *fname;

    filename = eval_string();
    if (!filename) {
        return;
    }

    fname = string_to_cstr(filename);
    if (!fname) {
        free_string(filename);
        return;
    }

    if (save_file(fname) != 0) {
        error(ERR_BAD_FILE);
    }

    free(fname);
    free_string(filename);
}

/*
 * SYSTEM statement - exit to shell
 */
void
do_system()
{
    cleanup();
    exit(0);
}

/*
 * SLEEP statement - pause execution for N tenths of a second
 * SLEEP 1 = 100ms, SLEEP 10 = 1 second
 */
void
do_sleep()
{
    int tenths;
#if defined(__211BSD__) || defined(pdp11) || !defined(__STDC__)
    struct timeval tv;
#endif

    skip_spaces();
    /* Check if there's an argument */
    if (peek_char() != '\0' && peek_char() != ':' &&
        peek_char() != TOK_ELSE) {
        tenths = eval_integer();
        if (tenths < 0) {
            tenths = 0;
        }
    } else {
        /* No argument - default to 1 tenth of a second */
        tenths = 1;
    }

#if defined(__211BSD__) || defined(pdp11) || !defined(__STDC__)
    /* 2.11 BSD - use select() for delay */
    tv.tv_sec = tenths / 10;
    tv.tv_usec = (tenths % 10) * 100000L;
    select(0, 0, 0, 0, &tv);
#else
    /* Modern systems - use usleep() */
    usleep((unsigned int)tenths * 100000);
#endif
}
