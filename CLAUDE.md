# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

PendVM is a virtual machine that executes programs written in the PISA / PAL assembly language — the instruction set of the **Pendulum**, a reversible processor designed in the mid-1990s at the MIT AI lab by Carlin Vieri and Michael P. Frank; the simulator/assembler (PendVM) was built by Matt Debergalis. The code is near-ANSI C (C89) and is preserved/rehosted rather than actively developed.

The defining feature of the target ISA is **reversibility**: every instruction must be undoable. The VM models this by carrying a direction flag (`FORWARD` / `REVERSE`) on the machine and by implementing destructive-update semantics (XOR-based writes, `EXCH` for memory, `SWAPBR` for control flow) rather than normal MIPS-style overwrites.

### Which PISA?

"PISA" is ambiguous. Three references describe three different instruction sets:

- **Vieri 1999 (Appendix A of *Reversible Computer Engineering and Architecture*)** — the *hardware* PISA: 12-bit words, 8 GPRs, branches that exchange `PC` with a register. Targets the FLATTOP silicon.
- **Frank 1999 (Appendix B of *Reversibility for Efficient Computing*)** — the *simulator* PISA: 32-bit words, 32 GPRs, branches that accumulate into a `BR` register. Frank's Appendix B is explicitly captioned as "the 32-bit simulator/compiler version of the PISA instruction set that was used in the Pendulum simulator and the R language compiler" — **this is the dialect PendVM implements.**
- **Haulund 2016 (§2.4 of *Design and Implementation of a Reversible Object-Oriented Programming Language*)** — a re-documentation of Frank's simulator PISA, adding one instruction (`DATA c` for static storage) used by the ROOPL compiler. Haulund's grammar and PendVM's `instructions[]` table agree almost exactly.

The bundled `fib.pisa` is written in **Vieri's hardware PISA**, not Frank's simulator PISA — it uses `bez $ra $rb`, `rbez`, `rbltz`, and two-register `bltz`, none of which exist in PendVM's `instructions[]` table. It has never run on this VM in its shipped form.

See `docs/PISA_COMPLIANCE_PLAN.md` for the detailed conformance status and a triage plan.

## Build and run

```sh
make                     # builds the `pendvm` binary (gcc, -std=c89)
make clean               # removes objects and binary
./pendvm <file.pisa>     # run a PAL program
./pendvm --debug <file>  # interactive debugger REPL
./pendvm --radix 16 <f>  # set output radix for show/emit (10 or 16)
```

There is **no test suite** and no lint configuration. The bundled `fib.pisa` does *not* execute cleanly — it is written in Vieri's hardware PISA dialect (see "Which PISA?" above) and uses instructions PendVM does not implement. For a smoke test, write or use a program that sticks to the mnemonics in `pal_parse.c`'s `instructions[]` table.

`DEBUG=-DDEBUG` in the Makefile enables `IF_DEBUG(...)` tracing; remove it for a quieter build. `-Wno-pointer-sign` is intentional — the code relies on signed/unsigned char pointer interchange.

## Architecture

Translation pipeline for a `.pisa` / PAL source file:

1. **`main.c`** — CLI parsing (`parse_command_line`), then `init_machine` → `load_imem` → either `step_processor(-1)` (batch) or `loop()` (interactive). Also hosts the `EXTRACT(num,low,high)` bit-field helper and `sign_extend` used throughout the instruction handlers.
2. **`machine.c: load_imem`** — reads the PAL source line-by-line, requires the `;; pendulum pal file` header, strips `;` comments, handles `.start` directives and labels, and stores each line into the memory subsystem as either `MEM_INST` or `MEM_DATA`. Labels are also appended to the global `lt` label table.
3. **`machine.c: step_processor` / `execute_instruction`** — fetch/decode/execute loop. For each step it fetches `mem_get(PC)`, calls `parse_inst` (in `pal_parse.c`) to dispatch to an `i_*` handler, then calls `adjust_pc` which uses `BR` and `dir` to compute the next PC (see "Reversibility" below).
4. **`pal_parse.c`** — contains the `instructions[]` dispatch table mapping mnemonic → `(arg-type triple, handler fn)`. `parse_inst` validates argument shapes using the `REG/IMM/AMT/OFF/LOFF` tags from `pal_parse.h`, converts text operands via `parse_reg` / `parse_immed` / `parse_label`, then invokes the handler.
5. **`memory.c`** — instruction/data memory is a **linked list** of `MEMORY` nodes keyed by address, not a flat array. `mem_get(addr)` lazily allocates new nodes on first access. Each node carries a `label`, a `breakpoint` flag, a `value` (for data), and an `inst` + `args[3]` (for instructions).
6. **`commands.c`** — the `commands[]` table drives the interactive REPL in `loop()`: `break`, `clear`, `continue`, `dir`, `read`, `reg`, `run`, `set`, `state`, `step`, `unbreak`, `help`, `quit`.

### The `MACHINE` struct and reversibility

`MACHINE` (defined in `pendvm.h`) holds `PC`, `BR` (branch register — the accumulated PC delta used by paired branches), `dir` (current direction), `externaldir` (nominal direction, drives `time` counter), `reset`, 32 GPRs, and `time`.

Reversibility shapes most handler implementations:

- **Arithmetic that overwrites** (`i_add`, `i_addi`, `i_sub`) multiplies the operand by `m->dir` so reversing direction undoes the operation.
- **Logical ops** write via XOR (`i_andx`, `i_orx`, `i_xorx`, `i_sllx`, `i_srlx`, `i_sltx`, …) so they are their own inverse. The `X` suffix on mnemonics in `pal_parse.c` flags this XOR-writeback form.
- **Branches** accumulate into `BR` rather than writing `PC` directly; `adjust_pc` consumes `BR` on the next tick. `i_rbra` additionally flips `m->dir`.
- **`EXCH`** swaps a register with memory (exch with instruction memory is intentionally unsupported and returns `-4`). **`SWAPBR`** exchanges a direction-normalized `BR` with a register.
- **`START` / `FINISH`** only halt (return `-3`) when the direction matches (FORWARD for `START`, REVERSE for `FINISH`); otherwise they are no-ops so programs can be run in either direction.

### `SHOW` / `OUT` output protocol

`i_show` is stateful: the *first* invocation with `output_type == PTYPE_NONE` treats the register as a **type code** (int signed/unsigned, base 10/16, float fixed/exp, 4-char packed string, or newline), and the *next* invocation emits the value in that type. `PTYPE_NEWLINE` prints immediately. See the masks in `pendvm.h` (`PTYPE_*`, `INT_*`, `FLOAT_*`, `STRING_CHAR*`).

`i_emit` is the simpler escape hatch: prints the register in the global radix and zeros it.

### Known rough edges

The code is a 1990s artifact kept alive for archival purposes. Expect: minor `-Wall` warnings, mixed K&R / ANSI style, 16-char instruction/arg buffers (see `memory.h`), hardcoded Windows shims via `_WIN32`/`_WIN64` in `pal_parse.c`. Prefer minimal surgical changes over modernization unless the user asks.
