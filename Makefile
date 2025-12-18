# Makefile for GW-BASIC C port
# Cross-platform build system for 2.11 BSD, MacOS, and Linux

# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Program name
TARGET = gwbasic

# Source files
SRCS = main.c repl.c tokenize.c parse.c variables.c arrays.c strings.c \
       eval.c statements.c functions.c execute.c error.c

# Object files
OBJS = $(SRCS:.c=.o)

# Platform-specific settings
ifeq ($(UNAME_M),pdp11)
    # 2.11 BSD on PDP-11
    CC = cc
    CFLAGS = -O -D__211BSD__
    LDFLAGS =
    LIBS = -lm
    PLATFORM = 211BSD
    $(info Building for 2.11 BSD (PDP-11))
else ifeq ($(UNAME_S),Darwin)
    # MacOS
    CC = cc
    # K&R function definitions are intentional for 2.11 BSD compatibility
    CFLAGS = -Wall -Wextra -O2 -D__MACOS__ \
             -Wno-deprecated-non-prototype
    LDFLAGS =
    LIBS = -lm
    PLATFORM = MACOS
    $(info Building for MacOS)
else ifeq ($(UNAME_S),Linux)
    # Linux
    CC = gcc
    # K&R function definitions are intentional for 2.11 BSD compatibility
    CFLAGS = -Wall -Wextra -O2 -D__LINUX__ \
             -Wno-deprecated-non-prototype
    LDFLAGS =
    LIBS = -lm
    PLATFORM = LINUX
    $(info Building for Linux)
else
    # Unknown platform - try generic settings
    CC = cc
    CFLAGS = -O
    LDFLAGS =
    LIBS = -lm
    PLATFORM = UNKNOWN
    $(warning Unknown platform, using generic settings)
endif

# Default target
all: $(TARGET)
	@echo "Built $(TARGET) for $(PLATFORM)"

# Link target
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile .c files to .o files
.c.o:
	$(CC) $(CFLAGS) -c $<

# Dependencies (simplified - could use makedepend)
main.o: main.c gwbasic.h
repl.o: repl.c gwbasic.h
tokenize.o: tokenize.c gwbasic.h
parse.o: parse.c gwbasic.h
variables.o: variables.c gwbasic.h
arrays.o: arrays.c gwbasic.h
strings.o: strings.c gwbasic.h
eval.o: eval.c gwbasic.h
statements.o: statements.c gwbasic.h
functions.o: functions.c gwbasic.h
execute.o: execute.c gwbasic.h
error.o: error.c gwbasic.h

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Install (optional)
install: $(TARGET)
	@echo "Installing $(TARGET)..."
	cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling $(TARGET)..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete"

# Help
help:
	@echo "GW-BASIC C Port Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build gwbasic (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Detected platform: $(PLATFORM)"

.PHONY: all clean install uninstall help
