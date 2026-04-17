# PendVM Test Suite — Implementation Report

## What was built

A two-tier test suite added entirely under `tests/`, with only the `Makefile` modified in the existing codebase.

**Unit tests (C, Unity, 78 passing):**

| File                | Covers                                                            | Count |
| ------------------- | ----------------------------------------------------------------- | ----- |
| `test_helpers.c`    | `power`, `EXTRACT`, `sign_extend`                                 | 3     |
| `test_arith.c`      | ADD / ADDI / SUB — direction-multiplied inverse                   | 7     |
| `test_logic.c`      | AND/OR/XOR/NOR/SLT + immediate forms — XOR-writeback self-inverse | 19    |
| `test_shift.c`      | SLLX/SRLX/SRAX + V-variants — self-inverse                        | 13    |
| `test_rotate.c`     | RL/RR/RLV/RRV — direction-gated round-trip                        | 7     |
| `test_involutive.c` | NEG / EXCH / SWAPBR                                               | 7     |
| `test_branch.c`     | BR accumulation, RBRA dir flip, START/FINISH gated halt           | 12    |
| `test_parse.c`      | `parse_reg` / `parse_immed` / `parse_inst` dispatch               | 10    |

**Integration harness (`tests/check_reversible`):** standalone binary. Initializes the VM, snapshots the `MACHINE` struct and data memory `[0..4096)`, runs forward to `FINISH`, flips direction (mirroring `commands.c:com_dir`), runs reverse to `START`, takes a second snapshot, and compares. Exits 0 on match, 1 on divergence.

**Integration programs (`tests/programs/*.pisa`):** `nop`, `arith`, `logic`, `rotate`, `branch` (paired-BRA), and `break` (deliberately irreversible — negative-path check).

**Build targets:** `make test`, `make test-unit`, `make test-integration`. `make test` exits 0.

## Bugs identified and how they were handled

### 1. Baseline build broken on modern clang (NEW — fixed)

The codebase wouldn't compile at all on Apple clang 17. Implicit function declarations (e.g. `strcmp` used without `#include <string.h>`) and int/pointer conversions that older toolchains only warned about are now hard errors by default.

**Handled:** added `-Wno-implicit-function-declaration -Wno-int-conversion` to `CFLAGS` in the `Makefile`. No source changes. This is the only deviation from "only additive Makefile changes" and is the minimum required to make the baseline build at all.

### 2. `i_srlx` performs arithmetic (not logical) right shift (NEW — pinned)

`machine.c:608-611`:
```c
m->reg[rd] ^= (m->reg[rs] >> amt);
```
`m->reg[]` is `int`, so `>> amt` is implementation-defined for negative values and does an arithmetic shift on every target we're likely to hit. Result: `SRLX` and `SRAX` produce identical output on inputs with the high bit set, even though the mnemonics imply different semantics.

**Handled:** pinned as `test_srlx_actually_arithmetic_CURRENT_BEHAVIOR` in `test_shift.c` with a comment pointing at the fix site. Anyone fixing the VM will see this test fail and update the assertion alongside their patch. Not fixed here — the plan is explicit that PendVM is an archival rehost, not an active project, and we prefer pinning to silent drift.

### 3. `parse_immed` endptr check is dead code (KNOWN — pinned)

`pal_parse.c:209-221`:
```c
char **endptr = NULL;
...
value = strtol(immed, endptr, 0);
if (endptr) return NULL;
```
`strtol` ignores a `NULL` endptr. The subsequent check tests the pointer-to-char** (which is `NULL`, always false) instead of `*endptr`. The "bad format" path is unreachable — garbage like `"12garbage"` parses as `12`.

**Handled:** pinned as `test_parse_immed_accepts_garbage_CURRENT_BEHAVIOR` in `test_parse.c` with a diagnostic message naming the fix site. Pre-existing and listed in the plan.

### 4. Missing `return` statements / fall-through (KNOWN — not touched)

Three functions are declared non-void but don't return a value on all paths:
- `machine.c:174-178` — `load_err`
- `machine.c:310-326` — `adjust_pc`
- `machine.c:333-369` — `execute_instruction` (falls off the end of the if/else chain when an unknown status is returned)
- `commands.c:213` — `com_run` (when the user declines to restart)

Latent undefined behavior; callers happen to not observe the garbage return value in current code paths.

**Handled:** out of scope per the plan. `-Werror=return-type` is *not* added so existing code keeps building. Left as-is; noted in the plan's "rough edges" section.

### 5. `fib.pisa` uses unimplemented instructions (KNOWN — worked around)

The bundled example uses `bez` / `rbez`, which are not in `pal_parse.c`'s `instructions[]` table. `./pendvm fib.pisa` errors at address 0x08. This predates this work and remains broken.

**Handled:** the test suite does not depend on `fib.pisa`. Our integration programs are authored against the instructions that *are* implemented (confirmed by reading `pal_parse.c:29-71`). `./pendvm fib.pisa` is untouched and still errors, exactly as documented in the plan's verification section.

### 6. `mem_get` does not zero-init instruction fields (KNOWN — not pinned)

`memory.c:32-42` leaves `inst[16]` and `args[3][16]` uninitialized on freshly-allocated cells; `MEM_EMPTY` cells therefore carry stack garbage in those fields. Only visible through `display_state`.

**Handled:** noted but not pinned. The integration harness calls `mem_get` over `[0..4096)` to snapshot the `.value` field, which is zero-initialized and safe. The garbage in `inst[]` / `args[]` for empty cells would only matter for the REPL display path, which no test exercises.

## Design notes / deviations

- **`main_for_tests.o`:** `EXTRACT`, `power`, `sign_extend`, `progname`, and `output_radix` live in `main.c` alongside the program's own `main()`. Test binaries can't link `main.o` directly without multiple-`main` errors. Rather than refactor `main.c` (plan forbids) or duplicate those symbols in a stub (would mean tests don't verify real code), I added one Makefile rule that compiles `main.c` with `-Dmain=pendvm_app_main` into `tests/main_for_tests.o`. Purely additive.

- **`dir` / `externaldir` excluded from reversibility comparison:** the harness itself flips them once between the forward and reverse runs, so they are necessarily inverted at the end. The exclusion is documented in `check_reversible.c` alongside the comparison code.

- **Negative-path convention:** `run_integration.sh` treats any program whose filename starts with `break` as expected-to-diverge. This keeps the "assert the harness actually catches bad programs" check inside the same `make test` run, so a regression that makes `check_reversible` always-pass would fail the suite.

## Verification

```
make clean && make             # baseline builds (with CFLAGS note above)
make test                      # 78 unit tests pass; 6 integration programs behave correctly
./pendvm fib.pisa              # exits 1 — pre-existing bez/rbez issue, unchanged
./tests/check_reversible tests/programs/{nop,arith,logic,rotate,branch}.pisa  # all exit 0
./tests/check_reversible tests/programs/break.pisa                            # exit 1 (deliberate)
```