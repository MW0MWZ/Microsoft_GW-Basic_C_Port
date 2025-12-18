# GW-BASIC C Port - Project Summary

## Overview

A portable C implementation of Microsoft GW-BASIC (1983). The code is written in **K&R C v2** for compatibility with older C compilers.

## Project Structure

```
gwbasic-c/
├── gwbasic.h           # Main header with data structures and prototypes
├── main.c              # Entry point and initialization
├── repl.c              # Interactive REPL and file I/O
├── tokenize.c          # BASIC tokenizer (text -> tokens)
├── parse.c             # Program line storage management
├── eval.c              # Expression evaluator with operator precedence
├── execute.c           # Main execution engine
├── statements.c        # BASIC statement implementations
├── functions.c         # Built-in functions (math, string)
├── variables.c         # Variable storage and management
├── arrays.c            # Array handling with DIM support
├── strings.c           # String memory management
├── error.c             # Error handling and messages
├── Makefile            # Cross-platform build system
├── Makefile.bsd        # BSD make compatible build
├── README.md           # User documentation
├── examples/           # Example BASIC programs
└── test/               # Test BASIC programs
    └── run_all_tests.bas  # Automated test suite
```

## Implemented Features

### Core Language Features
- Line number-based program storage
- Tokenized internal representation
- Expression evaluator with proper precedence
- Multiple data types (Integer, Single, Double, String)
- Variable type suffixes (%, !, #, $)
- Arrays with DIM statement

### BASIC Statements
- PRINT - Output with semicolons and commas
- INPUT - User input
- LET - Variable assignment
- IF/THEN/ELSE - Conditional execution
- GOTO - Unconditional jump
- GOSUB/RETURN - Subroutines
- FOR/NEXT - Counted loops
- WHILE/WEND - Conditional loops
- DIM - Array declaration
- DATA/READ/RESTORE - Data storage
- REM - Comments
- END, STOP - Program termination

### Program Management
- NEW - Clear program
- LIST - Display program
- RUN - Execute program
- CONT - Continue after STOP
- LOAD - Load from file
- SAVE - Save to file
- SYSTEM - Exit to shell

### Operators
- Arithmetic: +, -, *, /, ^, \, MOD
- Comparison: =, <>, <, >, <=, >=
- Logical: AND, OR, XOR, NOT

### Built-in Functions

**Math Functions:**
- ABS(x) - Absolute value
- INT(x) - Integer part
- SGN(x) - Sign of number
- SQR(x) - Square root
- RND(x) - Random number
- SIN(x), COS(x), TAN(x) - Trigonometric
- ATN(x) - Arctangent
- LOG(x) - Natural logarithm
- EXP(x) - Exponential

**String Functions:**
- LEN(s$) - String length
- ASC(s$) - ASCII code of first character
- CHR$(n) - Character from ASCII code
- STR$(x) - Number to string
- VAL(s$) - String to number
- LEFT$(s$,n) - Leftmost n characters
- RIGHT$(s$,n) - Rightmost n characters
- MID$(s$,start,len) - Substring
- INSTR(start,s1$,s2$) - Find substring

## Build System

The Makefile automatically detects the platform using `uname`:

- **macOS:** cc with -Wall -Wextra -O2
- **Linux:** gcc with -Wall -Wextra -O2
- **BSD:** Use Makefile.bsd with traditional make

All platforms link with -lm (math library).

## Code Quality

- **K&R C v2**: All variables declared at beginning of scope blocks
- **K&R function definitions**: Compatible with older C compilers
- **No modern C features**: No C99/C11 features used
- **16-bit safe**: Careful handling of integer sizes

## Testing

The project includes an automated test suite (`test/run_all_tests.bas`) that tests:
- Arithmetic operations
- Comparison operators
- Variable assignment
- Math functions (ABS, SGN, INT, SQR, SIN, COS, EXP, LOG)
- String functions (LEN, ASC, CHR$, LEFT$, RIGHT$, MID$, VAL, STR$)
- FOR/NEXT loops
- GOSUB/RETURN
- Arrays
- IF/THEN/ELSE
- Logical operators (AND, OR)

Run with: `printf "RUN\nSYSTEM\n" | ./gwbasic test/run_all_tests.bas`

All 43 tests pass on macOS.

## Known Limitations

- Graphics (SCREEN, PSET, LINE, CIRCLE, etc.)
- Sound (BEEP, PLAY, SOUND)
- Hardware I/O (PEEK, POKE, INP, OUT)
- Serial/Parallel port I/O

## Source Statistics

Total: 13 source files (~4,500 lines of C code)

## Usage Examples

**Interactive mode:**
```bash
$ ./gwbasic
GW-BASIC 3.23
(C) Copyright Microsoft 1983-1991
C Port (C) 2025 Andy Taylor
65536 Bytes free

Ok
PRINT "Hello!"
Hello!
Ok
```

**Run a file:**
```bash
$ ./gwbasic examples/hello.bas
```
