/*
 * gwbasic.h - Main header for GW-BASIC C port
 *
 * C Port Copyright (c) 2025 Andy Taylor
 * Original GW-BASIC Copyright (c) 1983 Microsoft Corporation
 *
 * K&R C v2 compatible - for 2.11 BSD, MacOS, and Linux
 */

#ifndef GWBASIC_H
#define GWBASIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__211BSD__) || defined(pdp11) || !defined(__STDC__)
#include <strings.h>  /* For index(), rindex(), bcopy() on BSD */
#endif
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

/* HUGE_VAL compatibility for 2.11 BSD and older systems */
#ifndef HUGE_VAL
#define HUGE_VAL 1.0e38
#endif

/* memmove compatibility for 2.11 BSD - use bcopy instead */
/* bcopy(src, dest, n) vs memmove(dest, src, n) - args reversed */
#if defined(__211BSD__) || defined(pdp11) || !defined(__STDC__)
#define memmove(dest, src, n) bcopy((src), (dest), (n))
#endif

/* strchr/strrchr compatibility for 2.11 BSD - use index/rindex */
/* Note: On 2.11 BSD, use index() directly instead of strchr() */

/* Platform detection */
#if defined(__211BSD__) || defined(pdp11)
#define PLATFORM_211BSD 1
#define IS_16BIT 1
#elif defined(__APPLE__) || defined(__MACH__)
#define PLATFORM_MACOS 1
#define IS_16BIT 0
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#define IS_16BIT 0
#else
#define PLATFORM_UNKNOWN 1
#define IS_16BIT 0
#endif

/* Basic constants */
#define LINLEN 80       /* Terminal line length */

/* Platform-specific buffer and name sizes for memory efficiency */
#if IS_16BIT
#define BUFLEN 80       /* Input buffer length - smaller for PDP-11 */
#define NAMLEN 10       /* Maximum variable name length - smaller for PDP-11 */
#else
#define BUFLEN 255      /* Input buffer length */
#define NAMLEN 40       /* Maximum variable name length */
#endif

/* Platform-specific limits for 16-bit vs 32-bit systems */
#if IS_16BIT
#define MAXLIN 32767    /* Maximum line number (16-bit signed int) */
#define PROGRAM_SIZE 16384L /* Program memory size - 16K for PDP-11 */
#define STACK_SIZE 16   /* FOR/GOSUB/WHILE stack size - smaller for PDP-11 */
#else
#define MAXLIN 65529    /* Maximum line number (32-bit) */
#define PROGRAM_SIZE 65536L /* Program memory size */
#define STACK_SIZE 50   /* FOR/GOSUB/WHILE stack size */
#endif

/* Data type indicators */
#define TYPE_INT    2   /* Integer (%) */
#define TYPE_SNG    4   /* Single precision (!) */
#define TYPE_DBL    8   /* Double precision (#) */
#define TYPE_STR    3   /* String ($) */

/* Token definitions */
#define TOK_END     0x81
#define TOK_FOR     0x82
#define TOK_NEXT    0x83
#define TOK_DATA    0x84
#define TOK_INPUT   0x85
#define TOK_DIM     0x86
#define TOK_READ    0x87
#define TOK_LET     0x88
#define TOK_GOTO    0x89
#define TOK_RUN     0x8A
#define TOK_IF      0x8B
#define TOK_RESTORE 0x8C
#define TOK_GOSUB   0x8D
#define TOK_RETURN  0x8E
#define TOK_REM     0x8F
#define TOK_STOP    0x90
#define TOK_PRINT   0x91
#define TOK_CLEAR   0x92
#define TOK_LIST    0x93
#define TOK_NEW     0x94
#define TOK_ON      0x95
#define TOK_WAIT    0x96
#define TOK_DEF     0x97
#define TOK_POKE    0x98
#define TOK_CONT    0x99
#define TOK_SLEEP   0x9A
#define TOK_CSAVE   0x9C
#define TOK_CLOAD   0x9D
#define TOK_OUT     0x9E
#define TOK_LPRINT  0x9F
#define TOK_LLIST   0xA0
#define TOK_WIDTH   0xA1
#define TOK_ELSE    0xA2
#define TOK_TRON    0xA3
#define TOK_TROFF   0xA4
#define TOK_SWAP    0xA5
#define TOK_ERASE   0xA6
#define TOK_EDIT    0xA7
#define TOK_ERROR   0xA8
#define TOK_RESUME  0xA9
#define TOK_DELETE  0xAA
#define TOK_AUTO    0xAB
#define TOK_RENUM   0xAC
#define TOK_DEFSTR  0xAD
#define TOK_DEFINT  0xAE
#define TOK_DEFSNG  0xAF
#define TOK_DEFDBL  0xB0
#define TOK_LINE    0xB1
#define TOK_WHILE   0xB2
#define TOK_WEND    0xB3
#define TOK_WRITE   0xB5
#define TOK_OPEN    0xB7
#define TOK_CLOSE   0xB8
#define TOK_LOAD    0xB9
#define TOK_MERGE   0xBA
#define TOK_SAVE    0xBB
#define TOK_SYSTEM  0xBD
#define TOK_CHAIN   0xBE
#define TOK_COMMON  0xBF

/* Function tokens */
#define TOK_TAB     0xFF84
#define TOK_TO      0xFF85
#define TOK_THEN    0xFF86
#define TOK_NOT     0xFF87
#define TOK_STEP    0xFF88
#define TOK_PLUS    0xFF89
#define TOK_MINUS   0xFF8A
#define TOK_MULT    0xFF8B
#define TOK_DIV     0xFF8C
#define TOK_POWER   0xFF8D
#define TOK_AND     0xFF8E
#define TOK_OR      0xFF8F
#define TOK_XOR     0xFF90
#define TOK_EQV     0xFF91
#define TOK_IMP     0xFF92
#define TOK_MOD     0xFF93
#define TOK_IDIV    0xFF94

/* Comparison operators */
#define TOK_GT      0xFF95
#define TOK_EQ      0xFF96
#define TOK_LT      0xFF97
#define TOK_GE      0xFF98
#define TOK_LE      0xFF99
#define TOK_NE      0xFF9A

/* Built-in functions */
#define TOK_SGN     0xFF9D
#define TOK_INT     0xFF9E
#define TOK_ABS     0xFF9F
#define TOK_FRE     0xFFA1
#define TOK_SQR     0xFFA3
#define TOK_RND     0xFFA4
#define TOK_SIN     0xFFA5
#define TOK_LOG     0xFFA6
#define TOK_EXP     0xFFA7
#define TOK_COS     0xFFA8
#define TOK_TAN     0xFFA9
#define TOK_ATN     0xFFAA
#define TOK_PEEK    0xFFAC
#define TOK_LEN     0xFFAD
#define TOK_STR     0xFFAE
#define TOK_VAL     0xFFAF
#define TOK_ASC     0xFFB0
#define TOK_CHR     0xFFB1
#define TOK_LEFT    0xFFB2
#define TOK_RIGHT   0xFFB3
#define TOK_MID     0xFFB4
#define TOK_INSTR   0xFFB5

/* Error codes */
#define ERR_NONE         0
#define ERR_NEXT_NO_FOR  1
#define ERR_SYNTAX       2
#define ERR_RETURN       3
#define ERR_OUT_OF_DATA  4
#define ERR_ILLEGAL_FUNC 5
#define ERR_OVERFLOW     6
#define ERR_OUT_OF_MEM   7
#define ERR_UNDEF_LINE   8
#define ERR_SUBSCRIPT    9
#define ERR_REDIM       10
#define ERR_DIV_ZERO    11
#define ERR_TYPE_MISM   13
#define ERR_OUT_OF_STR  14
#define ERR_STRING_LONG 15
#define ERR_STRING_COMP 16
#define ERR_CANT_CONT   17
#define ERR_UNDEF_USER  18
#define ERR_NO_RESUME   19
#define ERR_RESUME_NOERR 20
#define ERR_BAD_FILE    52
#define ERR_FILE_NOTFND 53
#define ERR_BAD_MODE    54

/* Forward declarations */
typedef struct line_s line_t;
typedef struct var_s var_t;
typedef struct array_s array_t;
typedef struct string_s string_t;
typedef union value_u value_t;

/* Value union for all BASIC types */
union value_u {
    int intval;         /* Integer value */
    float sngval;       /* Single precision */
    double dblval;      /* Double precision */
    string_t *strval;   /* String pointer */
};

/* String descriptor */
struct string_s {
    int len;            /* String length */
    char *ptr;          /* Pointer to string data */
};

/* Variable entry */
struct var_s {
    char name[NAMLEN+1]; /* Variable name */
    int type;            /* Data type */
    value_t value;       /* Value */
    var_t *next;         /* Next in list */
};

/* Array descriptor */
struct array_s {
    char name[NAMLEN+1]; /* Array name */
    int type;            /* Element type */
    int ndims;           /* Number of dimensions */
    int dims[8];         /* Dimension sizes */
    int size;            /* Total elements */
    value_t *data;       /* Array data */
    array_t *next;       /* Next array */
};

/* Program line structure */
struct line_s {
    int linenum;        /* Line number */
    int len;            /* Line length including this header */
    unsigned char text[1]; /* Tokenized text (flexible array) */
};

/* FOR loop stack entry */
typedef struct {
    int linenum;        /* Line number of FOR */
    unsigned char *text; /* Position in line */
    char varname[NAMLEN+1]; /* Loop variable */
    double limit;       /* TO value */
    double step;        /* STEP value */
} forstack_t;

/* GOSUB stack entry */
typedef struct {
    int linenum;        /* Return line */
    unsigned char *text; /* Return position */
} gosubstack_t;

/* WHILE loop stack entry */
typedef struct {
    int linenum;        /* WHILE line number */
    unsigned char *text; /* WHILE position */
} whilestack_t;

/* Global interpreter state */
typedef struct {
    unsigned char *txttab;  /* Start of program text */
    unsigned char *vartab;  /* Start of variables */
    unsigned char *arytab;  /* Start of arrays */
    unsigned char *strend;  /* End of arrays */
    unsigned char *fretop;  /* Top of free memory */
    unsigned char *memsiz;  /* End of memory */

    int curlin;             /* Current line number */
    unsigned char *txtptr;  /* Current text pointer */
    line_t *curline_ptr;    /* Pointer to current line_t for fast advance */

    var_t *varlist;         /* Variable list */
    var_t *lastvar;         /* Last accessed variable (cache) */
    array_t *arrlist;       /* Array list */

    forstack_t forstack[STACK_SIZE];  /* FOR loop stack */
    int forsp;                         /* FOR stack pointer */

    gosubstack_t gosubstack[STACK_SIZE]; /* GOSUB stack */
    int gosubsp;                          /* GOSUB stack pointer */

    whilestack_t whilestack[STACK_SIZE]; /* WHILE stack */
    int whilesp;                          /* WHILE stack pointer */

    int datlin;            /* Current DATA line */
    unsigned char *datptr; /* Current DATA pointer */

    int errnum;            /* Error number */
    int errlin;            /* Error line */

    int running;           /* 1 if program running */
    int tracing;           /* 1 if TRON active */

    jmp_buf errtrap;       /* Error recovery */

    char inputbuf[BUFLEN+1]; /* Input buffer */

    /* Random number state */
    unsigned long rndseed;

} state_t;

/* Global state pointer */
extern state_t *g_state;

/* Function prototypes */

/* main.c */
int main(int argc, char **argv);
void init_state();
void cleanup();

/* repl.c */
void repl();
void execute_direct(char *line);
int load_file(const char *filename);
int save_file(const char *filename);

/* tokenize.c */
unsigned char *tokenize_line(const char *line, int *len);
char *detokenize_line(unsigned char *tokens);
int is_keyword(const char *word);

/* parse.c */
void parse_line(int linenum, const char *text);
void insert_line(int linenum, unsigned char *tokens, int len);
void delete_line(int linenum);
line_t *find_line(int linenum);
void list_program(int start, int end);
void new_program();

/* execute.c */
void run_program(int startline);
void execute_statement();
int get_linenum();
void skip_to_eol();

/* eval.c */
value_t eval_expr(int *type);
double eval_numeric();
string_t *eval_string();
int eval_integer();
int get_next_char();
int peek_char();
void skip_spaces();
int match_token(int token);
string_t *parse_string_literal();

/* variables.c */
var_t *find_variable(const char *name, int create);
void set_variable(const char *name, value_t val, int type);
value_t get_variable(const char *name, int *type);
void clear_variables();

/* arrays.c */
array_t *find_array(const char *name, int create);
void dimension_array(const char *name, int *dims, int ndims, int type);
value_t *array_element(const char *name, int *indices, int nindices);
void clear_arrays();

/* strings.c */
string_t *alloc_string(int len);
void free_string(string_t *str);
string_t *concat_strings(string_t *s1, string_t *s2);
string_t *copy_string(string_t *src);
int compare_strings(string_t *s1, string_t *s2);
string_t *string_from_cstr(const char *cstr);
char *string_to_cstr(string_t *str);

/* statements.c */
void do_print();
void do_input();
void do_let();
void do_if();
void do_goto();
void do_gosub();
void do_return();
void do_for();
void do_next();
void do_while();
void do_wend();
void do_dim();
void do_data();
void do_read();
void do_restore();
void do_end();
void do_stop();
void do_cont();
void do_new();
void do_list();
void do_run();
void do_load();
void do_save();
void do_system();
void do_sleep();

/* functions.c */
double fn_sgn(double x);
double fn_int(double x);
double fn_abs(double x);
double fn_sqr(double x);
double fn_rnd(double x);
double fn_sin(double x);
double fn_cos(double x);
double fn_tan(double x);
double fn_atn(double x);
double fn_log(double x);
double fn_exp(double x);
double fn_fre(double x);

int fn_len(string_t *s);
int fn_asc(string_t *s);
string_t *fn_chr(int n);
string_t *fn_str(double x);
double fn_val(string_t *s);
string_t *fn_left(string_t *s, int n);
string_t *fn_right(string_t *s, int n);
string_t *fn_mid(string_t *s, int start, int len);
int fn_instr(int start, string_t *s1, string_t *s2);

/* error.c */
void error(int errnum);
void syntax_error();
const char *error_message(int errnum);

#endif /* GWBASIC_H */
