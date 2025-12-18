/*
 * main.c - Main entry point for GW-BASIC
 *
 * K&R C v2 compatible
 */

#include "gwbasic.h"

/* Global state */
state_t *g_state = NULL;

/*
 * Initialize interpreter state
 */
void
init_state()
{
    int i;
    long memsize;

    g_state = (state_t *)malloc(sizeof(state_t));
    if (!g_state) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    /* Allocate memory for BASIC program and data */
    /* Try progressively smaller sizes if allocation fails */
    memsize = PROGRAM_SIZE;
    g_state->txttab = NULL;
    while (memsize >= 4096L && !g_state->txttab) {
        g_state->txttab = (unsigned char *)malloc((unsigned)memsize);
        if (!g_state->txttab) {
            memsize = memsize / 2;
        }
    }
    if (!g_state->txttab) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    /* Mark end of program first */
    g_state->txttab[0] = 0;
    g_state->txttab[1] = 0;

    /* Initialize memory pointers - vartab points AFTER the end marker */
    g_state->vartab = g_state->txttab + 2;
    g_state->arytab = g_state->txttab + 2;
    g_state->strend = g_state->txttab + 2;
    g_state->fretop = g_state->txttab + memsize;
    g_state->memsiz = g_state->txttab + memsize;

    /* Initialize state */
    g_state->curlin = 0;
    g_state->txtptr = NULL;
    g_state->curline_ptr = NULL;
    g_state->varlist = NULL;
    g_state->lastvar = NULL;
    g_state->arrlist = NULL;

    g_state->forsp = 0;
    g_state->gosubsp = 0;
    g_state->whilesp = 0;

    g_state->datlin = 0;
    g_state->datptr = NULL;

    g_state->errnum = 0;
    g_state->errlin = 0;

    g_state->running = 0;
    g_state->tracing = 0;

    g_state->rndseed = 1;

    /* Clear input buffer */
    for (i = 0; i < BUFLEN + 1; i++) {
        g_state->inputbuf[i] = '\0';
    }
}

/*
 * Cleanup and exit
 */
void
cleanup()
{
    if (g_state) {
        if (g_state->txttab) {
            free(g_state->txttab);
        }
        clear_variables();
        clear_arrays();
        free(g_state);
        g_state = NULL;
    }
}

/*
 * Main entry point
 */
int
main(argc, argv)
int argc;
char **argv;
{
    int result;

    /* Initialize interpreter */
    init_state();

    /* Print banner */
    printf("GW-BASIC 3.23\n");
    printf("(C) Copyright Microsoft 1983-1991\n");
    printf("C Port (C) 2025 Andy Taylor\n");
    printf("%ld Bytes free\n\n", (long)(g_state->fretop - g_state->strend));

    /* Check if a file was specified */
    if (argc > 1) {
        /* Load and run the specified file */
        result = load_file(argv[1]);
        if (result == 0) {
            /* File loaded successfully, run it */
            run_program(0);
        } else {
            fprintf(stderr, "Cannot load %s\n", argv[1]);
            cleanup();
            return 1;
        }
    } else {
        /* Enter interactive mode */
        repl();
    }

    cleanup();
    return 0;
}
