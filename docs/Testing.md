# PendVM Test Suite — Implementation Report

## What was built

A two-tier test suite under `tests/`, with matching source-level fixes in the existing codebase (see §1-§6 below). The suite now doubles as the regression gate for the PISA compliance plan (`docs/PISA_COMPLIANCE_PLAN.md`) — each `§3` coding bug has a pinned test, and the two whole-program tests exercise the Haulund-dialect calling convention end-to-end.

**Unit tests (C, Unity, 86 passing):**

| File                | Covers                                                                  | Count |
| ------------------- | ----------------------------------------------------------------------- | ----- |
| `test_helpers.c`    | `power`, `EXTRACT`, `sign_extend`                                       | 3     |
| `test_arith.c`      | ADD / ADDI / SUB — direction-multiplied inverse + add/sub identity      | 9     |
| `test_logic.c`      | AND/OR/XOR/NOR/SLT + immediate forms — XOR-writeback self-inverse       | 19    |
| `test_shift.c`      | SLLX/SRLX/SRAX + V-variants — self-inverse; logical-shift zero-fill     | 14    |
| `test_rotate.c`     | RL/RR/RLV/RRV — direction-gated round-trip                              | 7     |
| `test_involutive.c` | NEG / EXCH / SWAPBR — incl. EXCH-against-inst-memory error path         | 7     |
| `test_branch.c`     | BR accumulation, RBRA dir flip, START/FINISH gated halt                 | 12    |
| `test_parse.c`      | `parse_reg` / `parse_immed` range-check / `parse_inst` dispatch         | 13    |
| `test_fib.c`        | whole-program: bundled Haulund-dialect `fib.pisa` computes `fib(18)`    | 1     |
| `test_mult.c`       | whole-program: Frank Fig 8-2 `mult_frank.pisa` computes `m1*m2`         | 1     |

**Integration harness (`tests/check_reversible`):** standalone binary. Initializes the VM, snapshots the `MACHINE` struct and data memory `[0..4096)`, runs forward to `FINISH`, flips direction (mirroring `commands.c:com_dir`), runs reverse to `START`, takes a second snapshot, and compares. Exits 0 on match, 1 on divergence.

**Integration programs (`tests/programs/*.pisa` + bundled `fib.pisa`):** `nop`, `arith`, `logic`, `rotate`, `branch` (paired-BRA), `data` (DATA-directive round-trip), `mult_frank` (Frank Fig 8-2 with nested subroutines), `break` (deliberately irreversible — negative-path check), and the bundled `fib.pisa` (Haulund-dialect iterative Fibonacci).

**Build targets:** `make test`, `make test-unit`, `make test-integration`. `make test` exits 0. Green suite: 86 Unity tests + 9 integration runs (8 reversible including bundled `fib.pisa` + 1 intentional-divergence `break.pisa`) = 95 distinct checks.

## Bugs identified and how they were handled

### 1. Baseline build broken on modern clang (NEW — fixed)

The codebase wouldn't compile at all on Apple clang 17. Implicit function declarations (e.g. `strcmp` used without `#include <string.h>`) and int/pointer conversions that older toolchains only warned about are now hard errors by default.

**Handled:** added `-Wno-implicit-function-declaration -Wno-int-conversion` to `CFLAGS` in the `Makefile`. No source changes. This is the only deviation from "only additive Makefile changes" and is the minimum required to make the baseline build at all.

### 2. `i_srlx` performs arithmetic (not logical) right shift (NEW — fixed)

`machine.c:608-611` previously did `m->reg[rd] ^= (m->reg[rs] >> amt)`. `m->reg[]` is `int`, so `>> amt` was implementation-defined for negative values and did an arithmetic shift on every target we're likely to hit. Result: `SRLX` and `SRAX` produced identical output on inputs with the high bit set, even though the mnemonics imply different semantics.

**Fixed:** both `i_srlx` and `i_srlvx` now cast the source through `unsigned int` before shifting so the high bits zero-fill, matching Frank Appendix B's logical-shift semantics. `i_srlvx` also masks the reg-supplied amount to the 0..31 range Frank specifies. The pinned `test_srlx_actually_arithmetic_CURRENT_BEHAVIOR` regression has been replaced with `test_srlx_zero_fills_high_bits` and `test_srlvx_zero_fills_high_bits`, asserting the fixed semantics.

### 3. `parse_immed` endptr check is dead code (KNOWN — fixed)

`pal_parse.c:209-221` previously did `char **endptr = NULL; value = strtol(immed, endptr, 0); if (endptr) return NULL;` — `strtol` ignores a `NULL` endptr, and the subsequent check tested the pointer-to-char** (always `NULL`, always false) instead of `*endptr`. Garbage like `"12garbage"` parsed as `12`.

**Fixed** as part of §3 of the compliance plan: `parse_immed` now declares `char *endptr; ... parsed = strtol(immed, &endptr, 0); if (endptr == immed || endptr == NULL || *endptr != '\0') return NULL;` and range-checks the result against the caller-supplied signed bit-width. `test_parse.c` asserts rejection of trailing garbage, out-of-range literals, and bare `-`/`+`.

### 4. Missing `return` statements / fall-through (KNOWN — fixed)

Two non-void functions previously fell off the end on at least one control path:
- `execute_instruction` (`machine.c`) — fell through when a handler returned an unknown status code.
- `com_run` (`commands.c`) — fell through the first time it was invoked on a reset machine (when `m->reset` was already true, the outer `if` was skipped and the function returned garbage).

(The original report also listed `load_err` and `adjust_pc`, but those are declared `void` in the current code — the report misdiagnosed them.)

**Fixed:** `execute_instruction` now returns `EXEC_ERROR` for unknown status codes rather than falling off the end. `com_run` now runs `step_processor(-1)` + `display_state()` and returns 0 on the reset-machine path, matching the user-visible behaviour on subsequent invocations.

### 5. `fib.pisa` uses unimplemented instructions (KNOWN — fixed)

The bundled example previously used Vieri-hardware mnemonics `bez` / `rbez` / two-register `bltz`, none of which appear in `pal_parse.c`'s `instructions[]` table. `./pendvm fib.pisa` errored at address 0x08.

**Fixed** as part of §7 step 5 of the compliance plan: `fib.pisa` is now a Haulund-dialect iterative reversible Fibonacci that loads `N` from a labelled `DATA` cell via `EXCH` and computes `$6 = fib(18) = 2584`. `./pendvm fib.pisa` runs cleanly, `check_reversible` round-trips it in the integration harness, and `tests/test_fib.c` pins the forward-FINISH value and scratch-register hygiene.

### 6. `mem_get` does not zero-init instruction fields (KNOWN — fixed)

`memory.c:32-42` previously left `inst[16]` and `args[3][16]` uninitialized on freshly-allocated cells; `MEM_EMPTY` cells therefore carried stack garbage in those fields, visible through `display_state`.

**Fixed:** `mem_get` now zeroes `inst[0]` and `args[0..2][0]` alongside the existing `label[0]=0` / `value=0` initialisation, so freshly-allocated cells are deterministic rather than carrying whatever happened to be on `malloc`'s heap at the time.

## Design notes / deviations

- **`main_for_tests.o`:** `EXTRACT`, `power`, `sign_extend`, `progname`, and `output_radix` live in `main.c` alongside the program's own `main()`. Test binaries can't link `main.o` directly without multiple-`main` errors. Rather than refactor `main.c` (plan forbids) or duplicate those symbols in a stub (would mean tests don't verify real code), I added one Makefile rule that compiles `main.c` with `-Dmain=pendvm_app_main` into `tests/main_for_tests.o`. Purely additive.

- **`dir` / `externaldir` excluded from reversibility comparison:** the harness itself flips them once between the forward and reverse runs, so they are necessarily inverted at the end. The exclusion is documented in `check_reversible.c` alongside the comparison code.

- **Negative-path convention:** `run_integration.sh` treats any program whose filename starts with `break` as expected-to-diverge. This keeps the "assert the harness actually catches bad programs" check inside the same `make test` run, so a regression that makes `check_reversible` always-pass would fail the suite.

## Verification

```
make clean && make             # baseline builds (with CFLAGS note above)
make test                      # 86 unit tests pass; 8 reversible + 1 intentional-divergence + bundled fib all behave correctly
./pendvm fib.pisa              # exits 0 — Haulund-dialect rewrite, computes $6 = fib(18) = 2584
./tests/check_reversible tests/programs/{nop,arith,logic,rotate,branch,data,mult_frank}.pisa  # all exit 0
./tests/check_reversible fib.pisa                                             # exit 0 (bundled example round-trips)
./tests/check_reversible tests/programs/break.pisa                            # exit 1 (deliberate)
```

## Status (2026-04-19)

All six bugs above are fixed in tree and covered by pinned regressions. The suite is green: `make test` passes 86 unit tests and 9 integration programs. The PISA compliance plan (`docs/PISA_COMPLIANCE_PLAN.md`) §3.1–§3.8 coding bugs are all resolved; §3.9 (quote-aware comment stripper) is deferred as benign because no Frank/Haulund grammar element produces quoted content. §7 delivery steps 1–6 are delivered; step 7 (binary encoding) is deferred by design per §5.5.