# PISA Compliance Plan for PendVM

## References

Three documents define "PISA". They do not agree. The remediation work
depends on choosing which one PendVM is trying to implement.

1. **Vieri 1999 — the *hardware* PISA.** *Reversible Computer Engineering
   and Architecture*, Carlin J. Vieri, MIT PhD thesis, June 1999. Chapter 8
   (PCU state machine), Chapter 9 (detailed code trace), **Appendix A**
   ("The Pendulum Instruction Set Architecture (PISA)", pp. 150–161).
   This is the 12-bit / 8-register / PC-exchange-branch variant targeted at
   the FLATTOP silicon.

2. **Frank 1999 — the *simulator* PISA.** *Reversibility for Efficient
   Computing*, Michael P. Frank, MIT PhD thesis, June 1999. Chapter 8
   ("Design and programming of reversible processors"), **Appendix B**
   ("The Pendulum instruction set architecture (PISA)", pp. 255–273).
   Frank is explicit that Appendix B is "the 32-bit simulator/compiler
   version of the PISA instruction set that was used in the Pendulum
   simulator and the R language compiler" (caption of Figure B-1, p. 256)
   and that it differs from Vieri's hardware version: "Yet another improved
   and simplified version of PISA is being implemented now by Vieri for his
   Ph.D. dissertation research" (§8.1.2, p. 206). Frank and Vieri worked in
   the same MIT group; DeBergalis (PendVM's author) is acknowledged on
   p. 3 for "the Pendulum assembler and emulator".

3. **Haulund 2016 — a PendVM-compatible PAL dialect.** *Design and
   Implementation of a Reversible Object-Oriented Programming Language*,
   Tue Haulund, University of Copenhagen MS thesis, June 2016. §2.4 (PISA
   grammar and inversion rules), Chapter 4 (ROOPL→PAL compilation),
   Appendix A.2 ("PISA.hs"). Haulund states the version "is compatible with
   the Pendulum virtual machine, PendVM" (p. 18) and his `PISA.hs` emits
   files with the `;; pendulum pal file` header that `machine.c:103`
   requires.

**The central finding of reading all three together is that PendVM
already implements Frank 1999 Appendix B, essentially correctly.** The
remediation work framed by earlier drafts of this document — against
Vieri's Appendix A — was pointed at the wrong reference. Most "divergences"
between PendVM and Vieri are in fact correct implementations of Frank.

---

## 1. Scope and intent

1. Recognise Frank 1999 Appendix B as the specification PendVM implements.
2. Fix the defects that are unambiguous coding bugs under any reading.
3. Add the features Frank's Appendix B *does* require but PendVM lacks.
4. Decide whether to additionally support Vieri's hardware PISA (the dialect
   the bundled `fib.pisa` is written in) and/or Haulund's `DATA` extension.
5. Build a test suite driven by Frank's Appendix B operation expressions,
   Frank §8.3's multiplication example, and (conditionally on the dialect
   choice) Vieri's Chapter 9 Fibonacci trace.

Each task below is ordered so it can be landed as a small, test-covered
increment (as per `CLAUDE.md` "small incremental changes").

### 1.1 Three PISAs: which one is PendVM trying to be?

| Aspect | Vieri 1999 App. A (hardware) | Frank 1999 App. B (simulator) | Haulund 2016 | PendVM `instructions[]` |
|--------|------------------------------|-------------------------------|--------------|-------------------------|
| GPR count | 8 (`$0`..`$7`), 3-bit field | 32 (`$0`..`$31`), 5-bit field | unspecified; R-compiler and ROOPL use ≥32 | **32** (matches Frank) |
| Word width | 12 bits | 32 bits (`mod 2^32` throughout B.3) | unspecified | **32-bit C `int`** (matches Frank) |
| Immediate | 9-bit sign-extended | 16-bit signed, sign-extended to 32 | unspecified | **16-bit** via `parse_immed(..,16)` (matches Frank) |
| Branch model | `BEZ ra,rb`: swap `PC` with `GPR[ra]` | `BEQ ra,rb,off`: `BR += off*dir`; between-instruction rule `if (BR=0) pc+=dir else pc+=BR*dir` | label → offset → `BR` | **BR-delta with `dir`** multiplier (matches Frank line-for-line) |
| `BEZ`/`RBEZ`/`RBLTZ` | present | **absent** | absent | absent (matches Frank) |
| `BEQ`/`BNE`/`BGEZ`/`BGTZ`/`BLEZ`/`BLTZ` | absent | present (`reg,off` or `reg,reg,off`) | present | **present** (matches Frank) |
| `NEG` | assembler macro only (Ch.9 p.130) | **native instruction** (`NEG rsd` → `rsd ← (2^32 - rsd) mod 2^32`, self-inverse, p.262) | present | **native** (matches Frank) |
| `SUB`, `NORX`, `SLTX`, `SLTIX` | absent | present (Figures B-1, B-2) | absent (SUB absent; rest absent) | **present** (matches Frank) |
| `RBRA`, `SWAPBR` | absent/different | present (B.5) | present | **present** (matches Frank) |
| `RLV`, `RRV`, `SLLVX`, `SRLVX`, `SRAVX` | absent | present (variable-amount shifts/rotates) | present | **present** (matches Frank) |
| `RL`, `RR`, `SLLX`, `SRLX`, `SRAX` | 1-bit only | **by `amt` immediate** (Figures B-1, B-2) | by `c` immediate | **by amount** (matches Frank) |
| `READ`, `SHOW`, `EMIT` | absent | **present** (B.5 as primitive I/O) | absent | **present** (matches Frank; `SHOW` grew stateful type-dispatch in PendVM — this is the only genuine extension) |
| `START`, `FINISH` | absent | present (simulation endpoints) | present | **present** (matches Frank) |
| `DATA c` | absent | absent | **present** (Figure 2.7, §4.2) | **absent** (missing vs. Haulund) |
| Direction-flip-inverts-ISA | yes | yes (implicit; dir=−1 means run backwards) | Figure 2.8 summarises rules | yes (implemented by `*m->dir` in arithmetic handlers) |
| Output file header | n/a | n/a | `;; pendulum pal file` | required |

**Conclusion.** PendVM is a faithful implementation of **Frank 1999
Appendix B**. The remaining gaps against Frank are small: a few coding
bugs (§3) and `OUT`/`OUTPUT`/`SHOW`'s stateful type-dispatch protocol
(which is a PendVM enrichment of Frank's plain `SHOW reg`).

The bundled `fib.pisa` is written in **Vieri 1999 Appendix A** — a
different dialect. The repository has always shipped a test program and
a VM that implement different versions of PISA.

Haulund's dialect is 99% a re-documentation of Frank's Appendix B. Its
one substantive addition is the `DATA c` instruction for static storage
initialisation, which is *also* absent from Frank.

### 1.2 The bundled `fib.pisa` does not currently run

`CLAUDE.md` says "`./pendvm fib.pisa` ... confirm it halts normally". That
claim is **stale**:

```
$ ./pendvm fib.pisa
ERROR at address 0008: undefined instruction
```

`BEZ` is not in `pal_parse.c`'s `instructions[]` table, and correctly so:
`BEZ` is a Vieri-hardware instruction, and PendVM implements Frank's
simulator dialect. The file is a verbatim transcription of Vieri Ch.9
pp.130–131. It has never run on this VM (the uses of `bez`, `rbez`, and
two-register `bltz` are all Vieri-hardware syntax). Restoring a
working bundled self-test is a goal of this plan; the shape of the fix
depends on the dialect policy (§5).

---

## 2. Conformance status against Frank 1999 Appendix B

Frank's Appendix B is taken as the specification PendVM aims at. Where
PendVM and Appendix B disagree, the disagreement is called out by filename
and line.

### 2.1 Instructions present in Appendix B and present in PendVM

Implemented with matching argument shapes and semantics (verified against
Figures B-1, B-2, B-3 and the detailed B.3/B.4/B.5 operation expressions):

`ADD`, `ADDI`, `ANDX`, `ANDIX`, `NORX`, `NEG`, `ORX`, `ORIX`, `RL`, `RLV`,
`RR`, `RRV`, `SLLX`, `SLLVX`, `SLTX`, `SLTIX`, `SRAX`, `SRAVX`, `SRLX`,
`SRLVX`, `SUB`, `XOR` (`XOR` and `XORX`), `XORI` (`XORI` and `XORIX`),
`BEQ`, `BGEZ`, `BGTZ`, `BLEZ`, `BLTZ`, `BNE`, `BRA`, `RBRA`, `EXCH`,
`SWAPBR`, `SHOW` (as `OUT`/`OUTPUT`/`SHOW`), `EMIT`, `START`, `FINISH`.

### 2.2 Instructions in Appendix B absent from PendVM

- `READ reg` (Frank B.5 p. 272): "XORs the next word of data from the
  processor's canonical input device into register `reg`." Not in
  `instructions[]`. The simulator does not currently have an input stream.

### 2.3 PendVM behaviour that extends Appendix B

These should be documented but are out-of-spec extensions, not bugs:

- The `SHOW`/`OUT`/`OUTPUT` stateful type-dispatch protocol
  (`machine.c:~690`, type codes in `pendvm.h` as `PTYPE_*`). Frank's
  Appendix B spec is the one-liner "Copies `reg` to output stream." PendVM
  instead treats the *first* `SHOW` as a type code (int/float/string/newline,
  base 10/16, signed/unsigned, fixed/exp) and the *second* as the value.
  Keep, but clearly document as a PendVM-specific I/O protocol.
- `EMIT` zeroing the source register after emitting (see `machine.c:752`
  and the `break.pisa` integration test). Frank defines `EMIT reg` as
  "explicit, reversible removal of unwanted data" (B.5 p. 273) but does
  not mandate that the register be left zero; the zeroing is a PendVM
  convention that makes the intended entropy-removal semantics concrete.

### 2.4 Instructions absent from Appendix B but required by Haulund 2016

- `DATA c` — store immediate `c` in the current memory cell. Haulund uses
  this heavily for virtual function tables and static initialisation
  (§4.2, Figure 4.3). Without it, no ROOPL-compiled program can load.

### 2.5 Instructions present in Vieri App. A but absent from Appendix B / PendVM

Only relevant under dialect choice §5.1(A) or §5.1(C):

- `BEZ ra, rb` — "if `GPR[rb]=0^12` then `PC ↔ GPR[ra]`" (Vieri p. 154).
- `RBEZ ra, rb` — `BEZ` + direction toggle (Vieri p. 157).
- `RBLTZ ra, rb` — "if `GPR[rb]<0` then `PC ↔ GPR[ra]`; toggle dir"
  (Vieri p. 158).
- The two-register PC-exchange form of `BLTZ` (Vieri p. 155 has
  `BLTZ ra, rb`, not Frank's `BLTZ rb, off`).

These implement Vieri's PCU state-machine branch model (Tables 8.1 and 8.2
in Vieri's thesis) and are incompatible with Frank's `BR`-delta model —
they cannot coexist without a dialect flag.

---

## 3. Correctness bugs to fix regardless of the dialect question

These are defects under any reading of any of the three references.

### 3.1 `adjust_pc` has no explicit return
`machine.c:310` is declared to return `int` but has no `return` statement.
Its single caller (`execute_instruction`) ignores the return. Change the
signature to `void`, or return the new PC and use it.

### 3.2 `load_err` has no return either
`machine.c:174` — same issue, declared `int`, no return.

### 3.3 `parse_immed` never checks `len`
`pal_parse.c:206`. The `len` parameter is accepted but never range-checked.
The existing `/* ??? */` comments at `pal_parse.c:120` and `:139` admit
this. Implement: for Frank's signed 16-bit immediate, require
`-32768 ≤ n ≤ 32767`; for `AMT` (shift/rotate amounts), `0 ≤ n < 2^len`.

### 3.4 `parse_immed`'s `endptr` check is dead code
`pal_parse.c:220`: `endptr=NULL; value=strtol(immed, endptr, 0); if(endptr)`.
Passing `NULL` as the second argument means `endptr` is never written;
the subsequent check is dead code. Malformed literals (e.g. `123abc`)
silently parse to `123`. Fix by passing a real address.

### 3.5 `load_imem` scanf width
`machine.c:85` uses `sscanf(..., "%s%s%s%s%s", tmp[0]..tmp[4])` into
`char tmp[5][32]`. Unbounded `%s` into a fixed buffer is a classic
overflow — add width specifiers (e.g. `%31s`).

### 3.6 `.start` immediate parsing width
`machine.c:165` passes `len=32` to `parse_immed` for the `.start`
directive address, which is inconsistent with Frank's 16-bit immediates
(for which PendVM uses `len=16`). `.start` accepts a label or a program
address; handle it out-of-band rather than routing through `parse_immed`
with a phantom length.

### 3.7 `i_srax` arithmetic right shift relies on implementation-defined `>>`
`machine.c:578` relies on `>>` on a signed `int` producing an arithmetic
shift — implementation-defined under C89/C99/C11. The existing comment
"the absurdity of C forces me to do this" acknowledges it. The manual
`power(2, amt) - 1` mask is also brittle for `amt>=31`. Replace with a
portable cast-and-mask form.

### 3.8 `SHOW` float mode calls `printf("%f", int)`
`machine.c:697` passes `m->reg[r]` (an `int`) to `%f`. Undefined behaviour
— the bit pattern is not reinterpreted. Either drop float output (it is
a PendVM extension, not a Frank requirement) or properly alias via
`memcpy` into a `float`.

### 3.9 `;` comment stripper ignores quotes
`machine.c:181` truncates each source line at the first `;`. If a program
ever uses `;` inside a string literal (via a future `DATA` or via labels
containing `;`), the line is silently corrupted. Low-priority because no
current program hits it, but trivially fixable.

---

## 4. Missing features

### 4.1 `DATA c` static-storage directive (Haulund-required, Frank-absent)

Add an `i_data(WORD imm, WORD, WORD)` handler that stores `imm` into the
current cell's `value` field and marks the cell `MEM_DATA` rather than
`MEM_INST`. Register it in `instructions[]` with shape `{IMM, NIL, NIL}`.
Without this, ROOPL programs cannot load (Haulund §4.2; Figure 4.3 shows
classes emitting runs of labelled `DATA` cells for vtables).

This is independent of the Vieri-vs-Frank dialect choice; add it
unconditionally.

### 4.2 `READ reg` input instruction (Frank B.5 optional)

Frank includes `READ` for symmetry with `SHOW`. PendVM has no input
stream today. Defer unless a user actually needs it — flagging it rather
than implementing on spec.

### 4.3 `BEZ`/`RBEZ`/`RBLTZ` Vieri-hardware branches (optional, §5.1)

Only add if the user chooses dialect (A) or (C). See §5.1 and §5.2.

---

## 5. Policy decisions

### 5.1 Dialect target — **open, flag for user**

This is the single largest question.

- **(A) Frank-only.** Keep the current VM essentially unchanged; land
  §3 fixes and §4.1 `DATA`. Replace the bundled `fib.pisa` with a
  Frank-dialect version (using `BEQ`/`BGEZ`/`BLTZ` rather than
  `BEZ`/`BLTZ $ra $rb`). Preserves ROOPL/Haulund compatibility.
  Loses literal fidelity to Vieri's Ch.9 Fibonacci listing.
- **(B) Vieri-also.** Add Vieri-hardware instructions and a 12-bit /
  8-register execution mode under a flag (`--dialect=vieri`). Keep
  Frank as the default. In Vieri mode, `BEZ`/`RBEZ`/`RBLTZ` implement
  the PC-exchange semantics from Vieri Tables 8.1/8.2 and `BR` is not
  used; in Frank mode the current behaviour is preserved. The bundled
  `fib.pisa` runs under `--dialect=vieri`.
- **(C) Vieri-only rewrite.** Abandon Frank, implement Vieri's
  Appendix A strictly: 12-bit mask, 8 registers, PC-exchange branches,
  delete everything not in Vieri App. A. Would break ROOPL and every
  currently-working program under `tests/programs/`. Cannot be
  recommended without the user owning the tradeoff.

**Recommendation (for user consideration):** (A) is the minimum-risk
path that keeps the VM working and matches its authentic design
reference (Frank 1999). (B) is the scholarly-complete path that
rehabilitates the Ch.9 Fibonacci listing as a runnable artefact.

### 5.2 `BEZ`/`RBEZ`/`RBLTZ` implementation shape (conditional on §5.1)

Only relevant if (B) or (C). Frank's §8.2.2 and Vieri's §8.3 both
describe paired branches. The difference is the storage: Frank keeps a
single `BR` delta and pairs by programmer convention; Vieri uses the
register file as the come-from store and pairs through the PC-exchange.

Under (B) / (C), implement per Vieri Tables 8.1 and 8.2:

```
BEZ ra, rb:   if GPR[rb]==0  then { tmp=PC; PC=GPR[ra]; GPR[ra]=tmp; }
RBEZ ra, rb:  BEZ behaviour; then dir = !dir
RBLTZ ra, rb: if GPR[rb]<0   then { swap PC, GPR[ra]; dir = !dir }
```

`adjust_pc` must become dialect-aware: Frank-mode keeps the current
`BR`-delta arithmetic, Vieri-mode drops `BR` entirely and relies on the
register file.

### 5.3 Word width and register count — **resolved by Frank**

Not open questions any more. Frank Appendix B is explicit: 32-bit word,
5-bit register field (32 GPRs), 16-bit signed immediate. PendVM already
matches. **No change needed for Frank dialect.**

Under §5.1(B)/(C), Vieri-mode additionally enforces a 12-bit mask on
register writes and rejects `$8`..`$31`. This is mode-local behaviour,
not a VM-wide change.

### 5.4 Extension policy — **keep Frank extensions, clearly document**

All of `SUB`, `NEG`, `NORX`, `SLTX`, `SLTIX`, `BEQ`/`BNE`/`BGEZ`/
`BGTZ`/`BLEZ`/`BLTZ`, `BRA`, `RBRA`, `SWAPBR`, `RLV`/`RRV`, `SLLVX`/
`SRLVX`/`SRAVX`, `SHOW`/`EMIT`, `START`/`FINISH` are **part of
Frank's Appendix B** — they are not PendVM extensions. Earlier drafts
of this document incorrectly flagged them as extensions. The only
actual extensions are:

- `OUT` / `OUTPUT` as aliases for `SHOW` (cosmetic).
- `SHOW`'s stateful type-dispatch protocol (genuine extension, document).
- `EMIT` zeroing the register (convention; consider whether to keep).

### 5.5 Binary encoding — **open**

Neither Frank (B.2 says "we only describe the instructions from an
assembly language programmer's point of view" — p.255) nor Haulund
provides a bit-level layout. Vieri's Appendix A Table A.2 does. A
binary loader is out of scope for Frank-dialect compliance but would be
required if §5.1(B)/(C) is taken far enough to simulate the FLATTOP
hardware.

---

## 6. Comprehensive test suite

The existing suite (`tests/test_arith.c`, `test_branch.c`, `test_helpers.c`,
`test_involutive.c`, `test_logic.c`, `test_parse.c`, `test_rotate.c`,
`test_shift.c`, plus `check_reversible` and `tests/programs/*.pisa`)
covers the skeleton. Additions, organised by which reference they
validate against:

### 6.1 Per-instruction Frank 1999 Appendix B unit tests

For **every** instruction in Frank's Figures B-1/B-2/B-3, add a unit
test that:

1. Seeds inputs and runs the `i_*` handler directly.
2. Asserts the destination register matches Frank's operation
   expression, evaluated independently.
3. Calls the inversion (SUB for ADD, identical call for self-inverse
   ops) and asserts restoration of the pre-call snapshot.

Frank's inversion rules (B.3) and Haulund Figure 2.8 agree:
`ADD↔SUB`, `ADDI r c↔ADDI r -c`, `RL↔RR`, `RLV↔RRV`, every other
listed instruction is self-inverse.

### 6.2 Frank §8.3 multiplication whole-program test

Figure 8-2 (Frank p. 213) is a complete annotated PISA multiplication
routine using R0..R31, `SWAPBR`, `BRA`, `NEG`, `EXCH`, `ANDX`, `BEQ`,
`BNE`, `RL`, `ADD`, `ADDI`, `SUB`, `SLLVX`, `RBRA`. Add:

1. `tests/programs/mult_frank.pisa` transcribed verbatim from Figure 8-2
   with a small driver that loads two multiplicands and calls `mult`.
2. A test that runs it forward, asserts `R5 == m1 * m2` for a handful of
   inputs, and asserts the auxiliary registers (`R28..R31`) are zero on
   return.
3. A `check_reversible` round-trip.

### 6.3 Vieri Ch.9 Fibonacci test (conditional on §5.1(B)/(C))

Only land if dialect (B) or (C) is chosen:

1. Keep the bundled `fib.pisa` verbatim (it is an archival artefact).
2. Test asserts `$6 == 2584` on forward run, `$0..$5` and `$7` all
   zero at `finish` (per Vieri p. 134).
3. Parameterised variant: replace `addi $2 18` with a runtime input,
   verify `$6 == Fib(N)` for N in {0, 1, 2, 5, 10, 15, 18}.

### 6.4 Haulund ROOPL integration test (conditional on §4.1)

Once `DATA` lands, transcribe Haulund's Appendix B.2 `LinkedList.pal`
example (or one of the smaller `*.pal` files from his compiler's test
corpus) and assert it runs end-to-end.

### 6.5 Reversibility property tests

Extend `check_reversible` (currently whole-program) with:

1. **Per-instruction**: for each handler Frank marks reversible
   (Figure B-1/B-2/B-3), snapshot state, run forward once, run in
   reverse once, assert state matches.
2. **Cumulative**: for random sequences of 100 instructions drawn from
   the reversible-only subset, assert forward-then-reverse restores state.
3. **Known-irreversible detection**: run `break*.pisa` through the
   harness and assert the harness reports divergence.

### 6.6 Parser/assembler tests

Building on `test_parse.c`:

1. Every mnemonic in the `instructions[]` table parses with its
   documented argument shape.
2. Out-of-range immediates are **rejected** (after §3.3).
3. Malformed numeric literals are **rejected** (after §3.4).
4. Register names outside `$0..$31` are rejected.
5. Labels that resolve outside the program are rejected rather than
   silently becoming garbage addresses.
6. The `.start` directive sets PC correctly; missing `.start` leaves
   PC=0.

### 6.7 Encoding-table tests (only under §5.5)

Only if a binary loader is adopted. Vieri Appendix A Table A.2 is the
authoritative source; round-trip assemble/disassemble each opcode.

---

## 7. Delivery order (suggested)

1. **§3 coding bugs** — independent of everything else. One commit per fix.
2. **§6.1 Frank-spec unit tests against current behaviour** — most will
   pass immediately (because PendVM already matches Frank). The ones that
   fail will reveal any hidden divergences not caught above.
3. **§4.1 `DATA` instruction** — unlocks ROOPL programs. Small change.
4. **§6.2 multiplication test** — validates the bulk of the instruction
   set end-to-end.
5. **Replace `fib.pisa`** with a Frank-dialect rewrite so the bundled
   self-test is live again. (Under §5.1(A).) OR defer until step 7.
6. **§6.4 ROOPL integration** — once `DATA` is in place.
7. **§5.1 dialect decision** — if (B)/(C), land the Vieri mode under a
   flag, add `BEZ`/`RBEZ`/`RBLTZ`, and wire up the Ch.9 Fibonacci test
   (§6.3). If (A), this step is just "keep the Frank-dialect `fib.pisa`
   rewrite from step 5".
8. **§5.5 binary encoding** — deferred; not load-bearing.

Every step should leave `make && ./pendvm fib.pisa && tests/run_unit.sh
&& tests/run_integration.sh` green before moving on. Note that
`./pendvm fib.pisa` is currently **broken** (§1.2); it becomes a
meaningful gate from step 5 (or step 7, depending on dialect choice).
Until then, "green" means `tests/run_unit.sh` and
`tests/run_integration.sh` pass.
