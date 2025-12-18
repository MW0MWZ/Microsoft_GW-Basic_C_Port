# GW-BASIC C Port

A portable C implementation of Microsoft GW-BASIC (1983), written in K&R C v2 for compatibility with older C compilers and systems.

## Building

```bash
make
```

The Makefile automatically detects your platform and uses appropriate compiler flags.

For older BSD systems:
```bash
make -f Makefile.bsd
```

## Running

Interactive mode:
```bash
./gwbasic
```

Load and run a program:
```bash
./gwbasic program.bas
```

## Testing

Run the automated test suite:
```bash
printf "RUN\nSYSTEM\n" | ./gwbasic test/run_all_tests.bas
```

All 43 tests should pass with output ending in "ALL TESTS PASSED!"

## Supported Features

### Core BASIC Statements
- PRINT, INPUT, LET
- IF/THEN/ELSE
- GOTO, GOSUB/RETURN
- FOR/NEXT loops
- WHILE/WEND loops
- DIM (arrays)
- DATA/READ/RESTORE
- REM (comments)

### Program Control
- LIST - List program lines
- RUN - Run program
- NEW - Clear program
- END, STOP - End program execution
- CONT - Continue after STOP
- LOAD, SAVE - Load/save programs
- SYSTEM - Exit to shell

### Data Types
- Integer (%)
- Single precision float (!)
- Double precision float (#)
- String ($)

### Operators
- Arithmetic: +, -, *, /, ^, \, MOD
- Comparison: =, <>, <, >, <=, >=
- Logical: AND, OR, XOR, NOT

### Built-in Functions

Math: ABS, INT, SGN, SQR, RND, SIN, COS, TAN, ATN, LOG, EXP

Strings: LEN, ASC, CHR$, STR$, VAL, LEFT$, RIGHT$, MID$, INSTR

## Example Programs

See the `examples/` directory for sample programs.

### Hello World
```basic
10 PRINT "Hello, World!"
20 END
```

### Fibonacci Sequence
```basic
10 REM Fibonacci sequence
20 LET A=0
30 LET B=1
40 FOR I=1 TO 10
50 PRINT B
60 LET C=A+B
70 LET A=B
80 LET B=C
90 NEXT I
100 END
```

## Known Limitations

- Graphics commands not supported (no SCREEN, PSET, LINE, etc.)
- Sound commands not supported (no BEEP, PLAY, SOUND)
- Hardware-specific commands not supported (no PEEK, POKE, INP, OUT)
- Serial/parallel port I/O not supported

## License

This is a clean-room reimplementation for educational purposes. The original GW-BASIC was copyright Microsoft Corporation 1983-1991.

## Architecture

The interpreter consists of:
- **tokenize.c** - Converts BASIC source to tokenized format
- **parse.c** - Manages program line storage
- **eval.c** - Expression evaluator with operator precedence
- **execute.c** - Main execution loop
- **statements.c** - Implementation of BASIC statements
- **functions.c** - Built-in functions
- **variables.c**, **arrays.c**, **strings.c** - Data management
- **error.c** - Error handling

## Platform Compatibility

The code is written in K&R C v2 for compatibility with older C compilers:
- Variables declared at the start of scope blocks
- K&R style function definitions
- No C89/C99/C11 features
- 16-bit safe integer handling

Tested on:
- macOS (Apple clang)
- Linux (GCC)

Should also compile on older Unix systems with K&R C compilers.
