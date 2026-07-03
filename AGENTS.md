# Cobra Language - Agent Context

## Project Overview
Cobra is an indentation-based (Python-style) programming language compiled to x86-64 native executables. Written in C. Targets macOS and Linux.

## Key Decisions
- Indentation-based blocks; `{}` reserved for dict/struct init literals.
- Assembly: `.intel_syntax noprefix` (macOS `clang`/`as` compatible).
- Runtime: `_cobra_` prefix (`_cobra_print`, `_cobra_println`, etc.).
- String constants in `.cstring` section, collected during codegen.
- Stack frame: `sub rsp, N` where N = 16 + 8*var_count (rounded to 16). Scratch at `[rbp - 8]`, params at `[rbp - 8*(i+1)]`, vars at `[rbp - 16 - 8*i]`.
- FFI: `extern fn name` → no body, linker resolves. Parens optional for no-arg externs.
- Repo: `https://github.com/Xznder1984/Cobra.git`
- Website: https://xznder1984.github.io/Cobra/

## Current State - WORKING
- Lexer, parser, semantic analyzer, type checker, x86-64 codegen all operational.
- Working features: `print()`, `println()`, `print_int()`, `print_float()`, `let` variables, `return`, `if/else`, `while`, binary ops, string/integer/bool literals, `extern fn` FFI.
- The binary op codegen uses `[rbp - 8]` for scratch (not `push`/`pop`) to maintain stack alignment.
- Label counter split: `str_label_count` for strings, `label_count` for control flow.
- Built-in functions recognized by semantic analyzer (no "Undefined symbol" errors).
- CLI tool with `-l`/`-L` flags for linking external libraries.

## Not Yet Working (codegen not implemented for these AST nodes)
- `let mut` reassignment (assignment operator `=` not parsed)
- `for` loops (parser produces AST, codegen missing)
- Function calls to user-defined functions (only built-ins work)
- Function parameters cannot be read (stored to stack, not tracked for lookup)
- `continue` (no-op)
- `break` (no-op, doesn't actually exit loop)
- Lists, dicts, structs, arrays, classes, traits

## Build & Test
```
make build        # builds compiler, CLI, runtime
make clean        # full clean
./compiler/bin/cobrac file.cb -o file.s          # compile to assembly
clang -c file.s -o file.o                         # assemble
clang file.o runtime/libcobra_runtime.a -o file   # link
```

## Examples
- `examples/hello/` - prints "Hello, World!" and an integer
- `examples/calculator/` - arithmetic with let variables
- `examples/cli-tool/` - CLI demo using extern fn FFI
- `examples/loops/` - while loop with counter (simplified, no assignment yet)

## Release Workflow
`.github/workflows/release.yml`:
- Triggered by pushing `v*.*.*` tags
- Builds for linux + macos x86_64
- Packages binaries, runtime, installer into tarballs
- Builds VS Code extension .vsix
- Creates GitHub Release with all artifacts

## Quick Links
- Compiler: `compiler/src/`
- CLI: `cli/src/main.c`
- Runtime: `runtime/src/runtime.c`
- VS Code ext: `vscode-extension/`
- Website: `website/index.html`
- Branding: `branding/`
- CI: `.github/workflows/ci.yml`
- Pages: `.github/workflows/pages.yml`
- Release: `.github/workflows/release.yml`