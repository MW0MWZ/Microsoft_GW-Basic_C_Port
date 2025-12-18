/*
 * execute.c - Program execution engine
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* K&R C compatible character test - ctype macros may fail on old systems */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

/*
 * Skip to end of current line
 */
void
skip_to_eol()
{
    while (peek_char() != '\0') {
        get_next_char();
    }
}

/*
 * Get next line number from program
 */
int
get_linenum()
{
    unsigned char *p;
    line_t *line;
    line_t *current;

    /* Find current line */
    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;
        if (line->linenum == g_state->curlin) {
            /* Move to next line */
            p += line->len;
            if (p[0] == 0 && p[1] == 0) {
                return 0; /* End of program */
            }
            current = (line_t *)p;
            return current->linenum;
        }
        p += line->len;
    }

    return 0;
}

/*
 * Execute a single statement
 */
void
execute_statement()
{
    int token;
    int c;

    skip_spaces();

    /* Check for end of line */
    c = peek_char();
    if (c == '\0') {
        return;
    }

    /* Check for statement separator */
    if (c == ':') {
        get_next_char();
        skip_spaces();
        c = peek_char();
    }

    /* Check for end of line again */
    if (c == '\0') {
        return;
    }

    /* Check for token (high bit set) */
    if (c & 0x80) {
        token = get_next_char();

        /* Handle two-byte tokens */
        /* Note: use & 0xFF for K&R C where char may be signed */
        if ((token & 0xFF) == 0xFF) {
            token = (token << 8) | get_next_char();
        }

        /* Dispatch based on token */
        switch (token) {
            case TOK_PRINT:
                do_print();
                break;

            case TOK_INPUT:
                do_input();
                break;

            case TOK_LET:
                do_let();
                break;

            case TOK_IF:
                do_if();
                break;

            case TOK_GOTO:
                do_goto();
                break;

            case TOK_GOSUB:
                do_gosub();
                break;

            case TOK_RETURN:
                do_return();
                break;

            case TOK_FOR:
                do_for();
                break;

            case TOK_NEXT:
                do_next();
                break;

            case TOK_WHILE:
                do_while();
                break;

            case TOK_WEND:
                do_wend();
                break;

            case TOK_DIM:
                do_dim();
                break;

            case TOK_DATA:
                do_data();
                break;

            case TOK_READ:
                do_read();
                break;

            case TOK_RESTORE:
                do_restore();
                break;

            case TOK_END:
                do_end();
                break;

            case TOK_STOP:
                do_stop();
                break;

            case TOK_CONT:
                do_cont();
                break;

            case TOK_NEW:
                do_new();
                break;

            case TOK_LIST:
                do_list();
                break;

            case TOK_RUN:
                do_run();
                break;

            case TOK_LOAD:
                do_load();
                break;

            case TOK_SAVE:
                do_save();
                break;

            case TOK_SYSTEM:
                do_system();
                break;

            case TOK_REM:
                /* Comment - skip rest of line */
                skip_to_eol();
                break;

            case TOK_TRON:
                g_state->tracing = 1;
                break;

            case TOK_TROFF:
                g_state->tracing = 0;
                break;

            case TOK_SLEEP:
                do_sleep();
                break;

            default:
                /* Unknown token */
                printf("Unknown statement token: %02X\n", token);
                syntax_error();
                break;
        }

    } else if (IS_ALPHA(c)) {
        /* Might be variable assignment without LET */
        do_let();

    } else {
        syntax_error();
    }
}

/*
 * Run a program from specified line
 */
void
run_program(startline)
int startline;
{
    unsigned char *p;
    line_t *line;
    line_t *startline_ptr;
    line_t *next_line;
    int prev_line;

    /* Mark as running */
    g_state->running = 1;

    /* Find starting line */
    startline_ptr = NULL;
    if (startline == 0) {
        /* Start from beginning */
        p = g_state->txttab;
        if (p[0] == 0 && p[1] == 0) {
            /* Empty program */
            g_state->running = 0;
            return;
        }
        startline_ptr = (line_t *)p;
    } else {
        startline_ptr = find_line(startline);
        if (!startline_ptr) {
            error(ERR_UNDEF_LINE);
            g_state->running = 0;
            return;
        }
    }

    /* Set up execution context */
    g_state->curlin = startline_ptr->linenum;
    g_state->txtptr = startline_ptr->text;
    g_state->curline_ptr = startline_ptr;  /* Track line pointer for fast advance */

    /* Set up error handler */
    if (setjmp(g_state->errtrap) != 0) {
        /* Error occurred */
        if (g_state->errnum != ERR_NONE) {
            if (g_state->curlin >= 0) {
                printf("%s in %d\n", error_message(g_state->errnum), g_state->errlin);
            } else {
                printf("%s\n", error_message(g_state->errnum));
            }
            g_state->errnum = ERR_NONE;
        }
        g_state->running = 0;
        return;
    }

    /* Main execution loop */
    while (g_state->running) {
        /* Trace if enabled */
        if (g_state->tracing && g_state->curlin >= 0) {
            printf("[%d]\n", g_state->curlin);
        }

        /* Save current line to detect jumps */
        prev_line = g_state->curlin;

        /* Execute statements on current line */
        while (peek_char() != '\0' && g_state->running) {
            /* Save position before each statement */
            prev_line = g_state->curlin;

            execute_statement();

            /* If a jump occurred (GOTO, GOSUB, etc.), break and restart */
            if (g_state->curlin != prev_line) {
                break;
            }

            /* Check for statement separator */
            skip_spaces();
            if (peek_char() == ':') {
                get_next_char();
            } else {
                break;
            }
        }

        /* Move to next line ONLY if no jump occurred */
        if (g_state->running && g_state->curlin == prev_line) {
            /* Use cached line pointer for O(1) advance instead of O(n) search */
            line = g_state->curline_ptr;
            p = ((unsigned char *)line) + line->len;

            if (p[0] == 0 && p[1] == 0) {
                /* End of program */
                g_state->running = 0;
            } else {
                next_line = (line_t *)p;
                g_state->curlin = next_line->linenum;
                g_state->txtptr = next_line->text;
                g_state->curline_ptr = next_line;
            }
        }
    }
}
