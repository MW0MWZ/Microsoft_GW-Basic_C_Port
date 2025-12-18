/*
 * error.c - Error handling
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/*
 * Error message table
 */
static const char *error_messages[] = {
    "No error",
    "NEXT without FOR",
    "Syntax error",
    "RETURN without GOSUB",
    "Out of DATA",
    "Illegal function call",
    "Overflow",
    "Out of memory",
    "Undefined line number",
    "Subscript out of range",
    "Duplicate definition",
    "Division by zero",
    NULL,
    "Type mismatch",
    "Out of string space",
    "String too long",
    "String formula too complex",
    "Can't continue",
    "Undefined user function",
    "No RESUME",
    "RESUME without error",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Bad file number",
    "File not found",
    "Bad file mode"
};

/*
 * Get error message for error number
 */
const char *
error_message(errnum)
int errnum;
{
    int num_messages;

    num_messages = (int)(sizeof(error_messages)/sizeof(error_messages[0]));
    if (errnum >= 0 && errnum < num_messages) {
        if (error_messages[errnum]) {
            return error_messages[errnum];
        }
    }
    return "Unknown error";
}

/*
 * Raise an error
 */
void
error(errnum)
int errnum;
{
    g_state->errnum = errnum;
    g_state->errlin = g_state->curlin;

    /* If in direct mode, print error immediately */
    if (g_state->curlin == -1) {
        printf("%s\n", error_message(errnum));
        g_state->errnum = ERR_NONE;
    }

    /* Jump to error handler */
    longjmp(g_state->errtrap, 1);
}

/*
 * Syntax error helper
 */
void
syntax_error()
{
    error(ERR_SYNTAX);
}
