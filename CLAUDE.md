# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make              # compile → ./pl0_compiler
make run          # compile + interactive test menu
make test         # compile + run tests/experiment5_parser/e5_t1_correct.pl0
make clean        # remove obj/ and binary
```

Compile a single file (e.g. after editing one module):
```bash
gcc -Wall -g -Iinclude -c src/lexer.c -o obj/lexer.o
```

GCC (MinGW on Windows). `obj/` must exist for `.o` files. All `#include` paths are relative to project root via `-Iinclude`.

## Architecture

3-pass PL/0 compiler written in C for a university compiler-design course.

**Pipeline:** source code → Lexer → Parser → Semantic Analyzer → output (symbol table + 4-address quads)

### Module overview

| Module | File | Role |
|--------|------|------|
| Shared definitions | `include/common.h` | Token/Symbol/Quad structs, TokenType enum, size constants |
| Lexer | `src/lexer.c` + `src/lexer.h` | Tokenizes source, exposes `current_token` global and `next_token()` |
| Parser | `src/lr_parser.c` + `src/lr_parser.h` | Recursive-descent syntax check (named "LR" but actually uses recursive descent, not table-driven LR) |
| Semantic | `src/semantic.c` + `src/semantic.h` | Symbol-table management + 4-address quad generation via recursive-descent; type-checks read/call targets |
| Entry point | `src/main.c` | File I/O, test-case management by experiment, drives pipeline with selectable run modes |

### Key design details

- **Each pass is independent.** `main.c` calls `init_lexer()` → `next_token()` three separate times (once for lexer output, once for parsing, once for semantic analysis). Both `lr_parser.c` and `semantic.c` keep their own `lookahead` and `match()` — they do not share parser state.
- **Global token state:** `current_token` (global in lexer) holds the most recently read token. `next_token()` advances it. Parser/semantic consumers read `current_token` after calling `next_token()`.
- **Case-insensitive:** identifiers and keywords are lowercased by the lexer before comparison. PL/0 keywords: `const`, `var`, `procedure`, `if`, `then`, `while`, `do`, `call`, `read`, `write`, `begin`, `end`, `odd`.
- **Comment styles:** `//`, `/* */`, and Pascal-style `(* *)` are all supported.
- **Semantic analyzer** — `check_is_variable()` verifies an identifier is a declared `SYM_VAR` (used by `read` and assignment). `check_is_procedure()` verifies `SYM_PROC` (used by `call`). `check_declared()` only checks existence (used when reading an identifier's value, where both const and var are valid). Errors: duplicate declaration, undeclared identifier, type mismatch.
- **Error recovery in lr_parser.c:** on syntax error, the parser skips tokens until it finds a synchronizing token (`;`, `end`, `begin`, `.`, EOF), then tries to continue.

### Run modes

`main.c` defines three `RunMode` values that control which passes execute:

| Mode | Passes run | Used by |
|------|-----------|---------|
| `MODE_LEXER_ONLY` | Lexer only | 实验四 tests |
| `MODE_LEXER_PARSER` | Lexer + Parser | 实验五 tests |
| `MODE_FULL` | Lexer + Parser + Semantic | 实验六 tests, CLI mode |

### Test file structure

```
tests/
├── experiment4_lexer/       # 实验四 — 词法分析 (2 tests)
├── experiment5_parser/      # 实验五 — 语法分析 (5 tests)
├── experiment6_semantic/    # 实验六 — 语义分析 (5 tests)
└── custom/                  # user-defined .pl0 files
```

Tests are loaded from files (not hardcoded in `main.c`). The interactive menu groups tests by experiment and runs them in experiment order. Pass a file path as a CLI argument to compile any `.pl0` file directly: `./pl0_compiler path/to/file.pl0`.
