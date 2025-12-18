/*
 * repl.c - Read-Eval-Print Loop for interactive mode
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/*
 * Check if a line starts with a line number
 * Note: explicit range check instead of isdigit() for old K&R C compatibility
 */
static int
has_linenum(line)
const char *line;
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return (*line >= '0' && *line <= '9');
}

/*
 * Extract line number from start of line
 * Note: explicit range check instead of isdigit() for old K&R C compatibility
 */
static int
extract_linenum(line)
const char *line;
{
    int num;

    num = 0;
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    while (*line >= '0' && *line <= '9') {
        num = num * 10 + (*line - '0');
        line++;
    }
    return num;
}

/*
 * Skip past line number at start of line
 * Note: explicit range check instead of isdigit() for old K&R C compatibility
 */
static const char *
skip_linenum(line)
const char *line;
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    while (*line >= '0' && *line <= '9') {
        line++;
    }
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return line;
}

/*
 * Execute a direct command (no line number)
 */
void
execute_direct(line)
char *line;
{
    unsigned char *tokens;
    unsigned char *saved_txtptr;
    int saved_curlin;
    int len;

    /* Tokenize the line */
    tokens = tokenize_line(line, &len);
    if (!tokens) {
        return;
    }

    /* Save current execution context */
    saved_txtptr = g_state->txtptr;
    saved_curlin = g_state->curlin;

    /* Set up for direct execution */
    g_state->txtptr = tokens;
    g_state->curlin = -1; /* -1 indicates direct mode */
    g_state->running = 0;

    /* Execute the statement */
    if (setjmp(g_state->errtrap) == 0) {
        execute_statement();
    } else {
        /* Error occurred */
        if (g_state->errnum != ERR_NONE) {
            printf("%s\n", error_message(g_state->errnum));
            g_state->errnum = ERR_NONE;
        }
    }

    /* Restore context */
    g_state->txtptr = saved_txtptr;
    g_state->curlin = saved_curlin;

    /* Free tokenized line */
    free(tokens);
}

/*
 * Main REPL loop
 */
void
repl()
{
    char *line;
    int linenum;
    const char *text;
    unsigned char *tokens;
    int len;

    while (1) {
        /* Print prompt */
        if (g_state->curlin == -1 || !g_state->running) {
            printf("Ok\n");
        }

        /* Read line */
        if (fgets(g_state->inputbuf, BUFLEN, stdin) == NULL) {
            break;
        }

        /* Remove trailing newline */
        line = g_state->inputbuf;
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        /* Skip empty lines */
        while (*line == ' ' || *line == '\t') {
            line++;
        }
        if (*line == '\0') {
            continue;
        }

        /* Check if line has a line number */
        if (has_linenum(line)) {
            /* This is a program line */
            linenum = extract_linenum(line);
            text = skip_linenum(line);

            if (*text == '\0') {
                /* Delete this line */
                delete_line(linenum);
            } else {
                /* Add or replace this line */
                tokens = tokenize_line(text, &len);
                if (tokens) {
                    insert_line(linenum, tokens, len);
                    free(tokens);
                }
            }
        } else {
            /* This is a direct command */
            execute_direct(line);
        }
    }
}

/*
 * Load a BASIC program from a file
 */
int
load_file(filename)
const char *filename;
{
    FILE *fp;
    char line[BUFLEN+1];
    int linenum;
    const char *text;
    unsigned char *tokens;
    int len;

    fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        return -1;
    }

    /* Clear existing program */
    new_program();

    /* Read lines from file */
    while (fgets(line, BUFLEN, fp) != NULL) {
        /* Remove trailing newline and carriage return (CRLF handling) */
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }

        /* Skip empty lines and comments */
        text = line;
        while (*text == ' ' || *text == '\t') {
            text++;
        }
        if (*text == '\0' || *text == '\'') {
            continue;
        }

        /* Parse line */
        if (has_linenum(line)) {
            linenum = extract_linenum(line);
            text = skip_linenum(line);

            if (*text != '\0') {
                tokens = tokenize_line(text, &len);
                if (tokens) {
                    insert_line(linenum, tokens, len);
                    free(tokens);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

/*
 * Save a BASIC program to a file
 */
int
save_file(filename)
const char *filename;
{
    FILE *fp;
    unsigned char *p;
    line_t *line;
    char *text;
    int linenum;

    fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }

    /* Iterate through program lines */
    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;
        linenum = line->linenum;

        /* Detokenize and write line */
        text = detokenize_line(line->text);
        if (text) {
            fprintf(fp, "%d %s\n", linenum, text);
            free(text);
        }

        /* Move to next line */
        p += line->len;
    }

    fclose(fp);
    return 0;
}
