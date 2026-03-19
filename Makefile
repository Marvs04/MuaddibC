# Makefile — Muad'dib Build System
#
# WHY A MAKEFILE and NOT a shell script?
# A Makefile tracks DEPENDENCIES. It only recompiles files that changed.
# If you edit lexer.l, Make knows to regenerate lex.yy.c but not
# re-run Bison. A shell script would recompile everything every time.
#
# HOW TO USE:
#   make          → builds the muaddib executable
#   make test     → runs the test file
#   make clean    → removes all generated files
#   make debug    → builds with debug symbols (for gdb)

# ── COMPILER AND FLAGS ──────────────────────────────────────────────────────

CC     = gcc

# WHY -Wall -Wextra?
# These flags turn on ALL warnings and EXTRA warnings.
# Warnings are the compiler telling you "this compiles but looks suspicious".
# Treating warnings seriously catches bugs before they become problems.
# A pro always compiles with -Wall. We do too — we just started, but we
# know why.
CFLAGS = -Wall -Wextra -g

# WHY -lfl?
# The Flex runtime library (libfl) provides the default yywrap() implementation
# and other Flex internals. We link against it so the linker can find
# those symbols. On some systems this is -lfl, on others -ll.
LIBS   =

# ── OUTPUT ──────────────────────────────────────────────────────────────────
TARGET = muaddib

# ── GENERATED FILES ─────────────────────────────────────────────────────────
# These are created by Bison and Flex — not hand-written
BISON_C  = parser.tab.c
BISON_H  = parser.tab.h
FLEX_C   = lex.yy.c

# ── DEFAULT TARGET ──────────────────────────────────────────────────────────

# WHY list $(TARGET) as the first rule?
# Make builds the FIRST rule it finds when you run 'make' with no arguments.
# By putting $(TARGET) first, 'make' means 'make muaddib'.
$(TARGET): $(BISON_C) $(FLEX_C) symbols.c main.c
	$(CC) $(CFLAGS) -o $(TARGET) $(BISON_C) $(FLEX_C) symbols.c main.c $(LIBS)
	@echo ""
	@echo "Build complete. Run with: ./$(TARGET) test.mdb"

# ── BISON RULE ───────────────────────────────────────────────────────────────

# WHY -d flag for Bison?
# -d tells Bison to generate the HEADER FILE (parser.tab.h) in addition
# to the C file (parser.tab.c). The header contains the token definitions
# that lexer.l needs. Without -d, the lexer wouldn't know what IS_TOKEN
# or BECOMES are.
#
# WHY -v flag?
# -v generates parser.output, a human-readable description of the
# parser's state machine. It's invaluable for debugging shift/reduce
# conflicts. We always generate it even if we don't always read it.
$(BISON_C) $(BISON_H): parser.y
	bison -d -v parser.y

# ── FLEX RULE ────────────────────────────────────────────────────────────────

# WHY does the Flex rule DEPEND on $(BISON_H)?
# This is the critical dependency order:
#   1. Bison must run FIRST to generate parser.tab.h
#   2. Flex runs SECOND — it generates lex.yy.c which #includes parser.tab.h
# If Flex ran before Bison, lex.yy.c would try to include a file that
# doesn't exist yet. Make handles this by checking dependencies.
$(FLEX_C): lexer.l $(BISON_H)
	flex lexer.l

# ── TEST TARGET ──────────────────────────────────────────────────────────────

# WHY a separate test target?
# Running tests should be ONE command: 'make test'.
# It builds first (if needed), then runs, then shows you the output.
# Automation is always better than remembering commands.
test: $(TARGET)
	@echo "=== Running test.mdb ==="
	@echo ""
	./$(TARGET) test.mdb

# ── DEBUG TARGET ─────────────────────────────────────────────────────────────

debug: $(BISON_C) $(FLEX_C) symbols.c main.c
	$(CC) $(CFLAGS) -DDEBUG -o $(TARGET)_debug \
		$(BISON_C) $(FLEX_C) symbols.c main.c $(LIBS)
	@echo "Debug build: ./$(TARGET)_debug"

# ── CLEAN TARGET ─────────────────────────────────────────────────────────────

# WHY clean generated files and not just the binary?
# parser.tab.c, parser.tab.h, and lex.yy.c are ALL generated files.
# They should not be committed to version control (they're in .gitignore).
# Cleaning them ensures a truly fresh build from source.
clean:
	rm -f $(TARGET) $(TARGET)_debug
	rm -f $(BISON_C) $(BISON_H) $(FLEX_C)
	rm -f parser.output
	rm -f *.o
	@echo "Clean complete."

# WHY .PHONY?
# Make usually checks if a file with the target's name exists.
# If a file called 'clean' existed, 'make clean' would do nothing.
# .PHONY tells Make: "these targets are always commands, not files".
.PHONY: test debug clean
