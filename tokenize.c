/*
 * tokenize.c - Tokenizer for BASIC source code
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* Keyword table */
typedef struct {
    const char *keyword;
    int token;
} keyword_t;

static keyword_t keywords[] = {
    {"END", TOK_END},
    {"FOR", TOK_FOR},
    {"NEXT", TOK_NEXT},
    {"DATA", TOK_DATA},
    {"INPUT", TOK_INPUT},
    {"DIM", TOK_DIM},
    {"READ", TOK_READ},
    {"LET", TOK_LET},
    {"GOTO", TOK_GOTO},
    {"RUN", TOK_RUN},
    {"IF", TOK_IF},
    {"RESTORE", TOK_RESTORE},
    {"GOSUB", TOK_GOSUB},
    {"RETURN", TOK_RETURN},
    {"REM", TOK_REM},
    {"STOP", TOK_STOP},
    {"PRINT", TOK_PRINT},
    {"CLEAR", TOK_CLEAR},
    {"LIST", TOK_LIST},
    {"NEW", TOK_NEW},
    {"ON", TOK_ON},
    {"WAIT", TOK_WAIT},
    {"DEF", TOK_DEF},
    {"POKE", TOK_POKE},
    {"CONT", TOK_CONT},
    {"SLEEP", TOK_SLEEP},
    {"CSAVE", TOK_CSAVE},
    {"CLOAD", TOK_CLOAD},
    {"OUT", TOK_OUT},
    {"LPRINT", TOK_LPRINT},
    {"LLIST", TOK_LLIST},
    {"WIDTH", TOK_WIDTH},
    {"ELSE", TOK_ELSE},
    {"TRON", TOK_TRON},
    {"TROFF", TOK_TROFF},
    {"SWAP", TOK_SWAP},
    {"ERASE", TOK_ERASE},
    {"EDIT", TOK_EDIT},
    {"ERROR", TOK_ERROR},
    {"RESUME", TOK_RESUME},
    {"DELETE", TOK_DELETE},
    {"AUTO", TOK_AUTO},
    {"RENUM", TOK_RENUM},
    {"DEFSTR", TOK_DEFSTR},
    {"DEFINT", TOK_DEFINT},
    {"DEFSNG", TOK_DEFSNG},
    {"DEFDBL", TOK_DEFDBL},
    {"LINE", TOK_LINE},
    {"WHILE", TOK_WHILE},
    {"WEND", TOK_WEND},
    {"WRITE", TOK_WRITE},
    {"OPEN", TOK_OPEN},
    {"CLOSE", TOK_CLOSE},
    {"LOAD", TOK_LOAD},
    {"MERGE", TOK_MERGE},
    {"SAVE", TOK_SAVE},
    {"SYSTEM", TOK_SYSTEM},
    {"CHAIN", TOK_CHAIN},
    {"COMMON", TOK_COMMON},
    {"TAB", TOK_TAB},
    {"TO", TOK_TO},
    {"THEN", TOK_THEN},
    {"NOT", TOK_NOT},
    {"STEP", TOK_STEP},
    {"AND", TOK_AND},
    {"OR", TOK_OR},
    {"XOR", TOK_XOR},
    {"EQV", TOK_EQV},
    {"IMP", TOK_IMP},
    {"MOD", TOK_MOD},
    {"SGN", TOK_SGN},
    {"INT", TOK_INT},
    {"ABS", TOK_ABS},
    {"FRE", TOK_FRE},
    {"SQR", TOK_SQR},
    {"RND", TOK_RND},
    {"SIN", TOK_SIN},
    {"LOG", TOK_LOG},
    {"EXP", TOK_EXP},
    {"COS", TOK_COS},
    {"TAN", TOK_TAN},
    {"ATN", TOK_ATN},
    {"PEEK", TOK_PEEK},
    {"LEN", TOK_LEN},
    {"STR$", TOK_STR},
    {"VAL", TOK_VAL},
    {"ASC", TOK_ASC},
    {"CHR$", TOK_CHR},
    {"LEFT$", TOK_LEFT},
    {"RIGHT$", TOK_RIGHT},
    {"MID$", TOK_MID},
    {"INSTR", TOK_INSTR},
    /* Operators - needed for detokenization */
    {"=", TOK_EQ},
    {"+", TOK_PLUS},
    {"-", TOK_MINUS},
    {"*", TOK_MULT},
    {"/", TOK_DIV},
    {"^", TOK_POWER},
    {"\\", TOK_IDIV},
    {"<>", TOK_NE},
    {"<=", TOK_LE},
    {">=", TOK_GE},
    {"<", TOK_LT},
    {">", TOK_GT},
    {NULL, 0}
};

/*
 * Look up keyword in table
 */
int
is_keyword(word)
const char *word;
{
    int i;
    char upword[NAMLEN+1];
    char *p;
    const char *q;

    /* Convert to uppercase - use safe conversion for old systems */
    /* toupper() on K&R C may corrupt already-uppercase chars */
    p = upword;
    q = word;
    while (*q && p < upword + NAMLEN) {
        if (*q >= 'a' && *q <= 'z') {
            *p++ = *q - 'a' + 'A';
        } else {
            *p++ = *q;
        }
        q++;
    }
    *p = '\0';

    /* Search keyword table */
    for (i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(upword, keywords[i].keyword) == 0) {
            return keywords[i].token;
        }
    }

    return 0; /* Not a keyword */
}

/*
 * Check if character is valid in identifier
 */
static int
is_ident_char(c)
int c;
{
    /* Use explicit checks instead of isalnum for K&R C compatibility */
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c == '.' || c == '_') return 1;
    return 0;
}

/*
 * Tokenize a BASIC line
 * Returns malloced token buffer, caller must free
 */
unsigned char *
tokenize_line(line, len)
const char *line;
int *len;
{
    unsigned char *tokens;
    unsigned char *newtokens;
    unsigned char *p;
    const char *s;
    char word[NAMLEN+1];
    int i;
    int token;
    int in_string;
    int in_data;
    int in_rem;
    int allocated;
    int offset;

    /* Allocate token buffer */
    allocated = BUFLEN * 2;
    tokens = (unsigned char *)malloc(allocated);
    if (!tokens) {
        return NULL;
    }

    p = tokens;
    s = line;
    in_string = 0;
    in_data = 0;
    in_rem = 0;

    while (*s) {
        /* Check if we need more space */
        if (p - tokens >= allocated - 10) {
            offset = p - tokens;  /* Save position before realloc */
            allocated *= 2;
            newtokens = (unsigned char *)realloc(tokens, allocated);
            if (!newtokens) {
                free(tokens);
                return NULL;
            }
            tokens = newtokens;
            p = tokens + offset;  /* Restore position in new buffer */
        }

        /* Skip spaces (except in strings, DATA, REM) */
        if (!in_string && !in_data && !in_rem && (*s == ' ' || *s == '\t')) {
            s++;
            continue;
        }

        /* Handle string literals */
        if (*s == '"') {
            *p++ = *s++;
            in_string = !in_string;
            continue;
        }

        /* If in string, DATA, or REM, copy verbatim */
        if (in_string || in_data || in_rem) {
            *p++ = *s++;
            continue;
        }

        /* Handle REM and ' (comment) */
        if (*s == '\'') {
            *p++ = TOK_REM;
            s++;
            in_rem = 1;
            continue;
        }

        /* Check for keywords */
        /* Use explicit check instead of isalpha for K&R C compatibility */
        if ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z')) {
            i = 0;
            while (is_ident_char(*s) && i < NAMLEN) {
                word[i++] = *s++;
            }
            word[i] = '\0';

            token = is_keyword(word);
            if (token) {
                /* Store token */
                /* Note: use & 0xFF00 instead of > 0xFF for 16-bit signed int */
                if (token & 0xFF00) {
                    *p++ = (token >> 8) & 0xFF;
                }
                *p++ = token & 0xFF;

                /* Check for DATA or REM */
                if (token == TOK_DATA) {
                    in_data = 1;
                } else if (token == TOK_REM) {
                    in_rem = 1;
                }
            } else {
                /* Not a keyword, store as identifier */
                i = 0;
                while (word[i]) {
                    *p++ = word[i++];
                }
            }
            continue;
        }

        /* Handle numbers */
        /* Note: explicit range check instead of isdigit() for old K&R C */
        if ((*s >= '0' && *s <= '9') || (*s == '.' && s[1] >= '0' && s[1] <= '9')) {
            while ((*s >= '0' && *s <= '9') || *s == '.' || *s == 'E' || *s == 'e' ||
                   *s == '+' || *s == '-' || *s == 'D' || *s == 'd') {
                *p++ = *s++;
            }
            /* Check for type suffix */
            if (*s == '%' || *s == '!' || *s == '#' || *s == '$') {
                *p++ = *s++;
            }
            continue;
        }

        /* Handle operators and punctuation */
        if (*s == '>' && s[1] == '=') {
            *p++ = (TOK_GE >> 8) & 0xFF;
            *p++ = TOK_GE & 0xFF;
            s += 2;
        } else if (*s == '<' && s[1] == '=') {
            *p++ = (TOK_LE >> 8) & 0xFF;
            *p++ = TOK_LE & 0xFF;
            s += 2;
        } else if (*s == '<' && s[1] == '>') {
            *p++ = (TOK_NE >> 8) & 0xFF;
            *p++ = TOK_NE & 0xFF;
            s += 2;
        } else if (*s == '>') {
            *p++ = (TOK_GT >> 8) & 0xFF;
            *p++ = TOK_GT & 0xFF;
            s++;
        } else if (*s == '=') {
            *p++ = (TOK_EQ >> 8) & 0xFF;
            *p++ = TOK_EQ & 0xFF;
            s++;
        } else if (*s == '<') {
            *p++ = (TOK_LT >> 8) & 0xFF;
            *p++ = TOK_LT & 0xFF;
            s++;
        } else if (*s == '+') {
            *p++ = (TOK_PLUS >> 8) & 0xFF;
            *p++ = TOK_PLUS & 0xFF;
            s++;
        } else if (*s == '-') {
            *p++ = (TOK_MINUS >> 8) & 0xFF;
            *p++ = TOK_MINUS & 0xFF;
            s++;
        } else if (*s == '*') {
            *p++ = (TOK_MULT >> 8) & 0xFF;
            *p++ = TOK_MULT & 0xFF;
            s++;
        } else if (*s == '/') {
            *p++ = (TOK_DIV >> 8) & 0xFF;
            *p++ = TOK_DIV & 0xFF;
            s++;
        } else if (*s == '^') {
            *p++ = (TOK_POWER >> 8) & 0xFF;
            *p++ = TOK_POWER & 0xFF;
            s++;
        } else if (*s == '\\') {
            *p++ = (TOK_IDIV >> 8) & 0xFF;
            *p++ = TOK_IDIV & 0xFF;
            s++;
        } else {
            /* Copy other characters verbatim */
            *p++ = *s++;
        }
    }

    /* Add terminator */
    *p++ = '\0';

    *len = p - tokens;
    return tokens;
}

/*
 * Detokenize a line back to text
 */
char *
detokenize_line(tokens)
unsigned char *tokens;
{
    char *text;
    char *newtext;
    char *p;
    unsigned char *t;
    int i;
    int token;
    int allocated;
    int offset;

    allocated = BUFLEN * 2;
    text = (char *)malloc(allocated);
    if (!text) {
        return NULL;
    }

    p = text;
    t = tokens;

    while (*t) {
        /* Check if we need more space */
        if (p - text >= allocated - 40) {
            offset = p - text;  /* Save position before realloc */
            allocated *= 2;
            newtext = (char *)realloc(text, allocated);
            if (!newtext) {
                free(text);
                return NULL;
            }
            text = newtext;
            p = text + offset;  /* Restore position in new buffer */
        }

        /* Check for two-byte token */
        /* Note: use & 0xFF for K&R C where unsigned char may be signed */
        if ((*t & 0xFF) == 0xFF) {
            token = ((*t & 0xFF) << 8) | (*(t+1) & 0xFF);
            t += 2;

            /* Find keyword for this token */
            for (i = 0; keywords[i].keyword != NULL; i++) {
                if (keywords[i].token == token) {
                    strcpy(p, keywords[i].keyword);
                    p += strlen(keywords[i].keyword);
                    *p++ = ' ';
                    break;
                }
            }
        } else if (*t & 0x80) {
            /* Single-byte token (high bit set) */
            token = *t++ & 0xFF;

            /* Find keyword for this token */
            for (i = 0; keywords[i].keyword != NULL; i++) {
                if (keywords[i].token == token) {
                    strcpy(p, keywords[i].keyword);
                    p += strlen(keywords[i].keyword);
                    *p++ = ' ';
                    break;
                }
            }
        } else {
            /* Regular character */
            *p++ = *t++;
        }
    }

    *p = '\0';

    return text;
}
