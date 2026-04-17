/* check_reversible.c - integration harness for whole-program reversibility.
 *
 * Strategy:
 *   1. init_machine + load_imem(prog)
 *   2. snapshot A = (MACHINE state, data memory [0..mem_max))
 *   3. step_processor(-1), require EXEC_FINISH
 *   4. flip direction (mirrors commands.c:com_dir), adjust_pc
 *   5. step_processor(-1), require EXEC_FINISH
 *   6. snapshot B
 *   7. compare A == B; exit 0 on match, 1 on divergence
 *
 * Exit codes:
 *    0  program is reversible (state at END == state at START)
 *    1  program diverged, or runtime issue
 *    2  print_usage / argument error
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pendvm.h"
#include "memory.h"

/* progname and output_radix come from main_for_tests.o (main.c recompiled
 * with main() renamed), which is part of TEST_LINK_OBJS — that is also where
 * EXTRACT/power/sign_extend live. */
extern char *progname;
extern MACHINE *m;

#define DEFAULT_MEM_MAX 4096

typedef struct {
    MACHINE mach;
    WORD   *mem;          /* mem_max words */
    int     mem_max;
} SNAPSHOT;

static void die(const char *msg)
{
    fprintf(stderr, "check_reversible: %s\n", msg);
    exit(1);
}

static void snapshot_take(SNAPSHOT *s, int mem_max)
{
    int i;
    s->mach = *m;
    s->mem_max = mem_max;
    s->mem = (WORD *)malloc(sizeof(WORD) * mem_max);
    if (!s->mem) die("out of memory");
    for (i = 0; i < mem_max; i++) {
        MEMORY *cell = mem_get((WORD)i);
        /* snapshot only the value field; data words and fresh/empty cells
         * both carry their state there. instruction memory has value==0
         * and we compare it the same way in both snapshots. */
        s->mem[i] = cell->value;
    }
}

static int snapshot_eq(const SNAPSHOT *a, const SNAPSHOT *b)
{
    int i;
    int diffs = 0;

    if (a->mach.PC != b->mach.PC) {
        fprintf(stderr, "  PC differs: %08X -> %08X\n", a->mach.PC, b->mach.PC);
        diffs++;
    }
    if (a->mach.BR != b->mach.BR) {
        fprintf(stderr, "  BR differs: %08X -> %08X\n", a->mach.BR, b->mach.BR);
        diffs++;
    }
    /* dir and externaldir are deliberately NOT compared: the harness
     * flips them once between the forward and reverse runs, so after the
     * round trip they will be inverted relative to the initial snapshot.
     * That inversion is by construction, not a reversibility failure. */

    for (i = 0; i < MAX_REG; i++) {
        if (a->mach.reg[i] != b->mach.reg[i]) {
            fprintf(stderr, "  $%d differs: %08X -> %08X\n",
                    i, (unsigned)a->mach.reg[i], (unsigned)b->mach.reg[i]);
            diffs++;
        }
    }

    for (i = 0; i < a->mem_max; i++) {
        if (a->mem[i] != b->mem[i]) {
            fprintf(stderr, "  mem[%04X] differs: %08X -> %08X\n",
                    i, a->mem[i], b->mem[i]);
            diffs++;
        }
    }

    return diffs == 0;
}

static void print_usage(void)
{
    fprintf(stderr,
            "print_usage: check_reversible [--mem-max N] <program.pisa>\n"
            "  runs the program forward to FINISH, then reverses\n"
            "  and re-runs to START, then compares state.\n");
    exit(2);
}

int main(int argc, char **argv)
{
    SNAPSHOT before, after;
    char *input = NULL;
    int mem_max = DEFAULT_MEM_MAX;
    int i;
    int status;

    progname = argv[0];

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--mem-max")) {
            if (++i >= argc) print_usage();
            mem_max = atoi(argv[i]);
            if (mem_max <= 0) print_usage();
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_usage();
        } else if (input) {
            print_usage();
        } else {
            input = argv[i];
        }
    }
    if (!input) print_usage();

    init_machine();
    load_imem(input);

    snapshot_take(&before, mem_max);

    status = step_processor(-1);
    if (status != EXEC_FINISH) {
        fprintf(stderr,
                "check_reversible: forward run did not reach FINISH "
                "(step_processor returned %d)\n", status);
        return 1;
    }

    /* Mirror commands.c:com_dir when flipping direction externally. */
    m->BR = -m->BR;
    m->dir = (m->dir == FORWARD) ? REVERSE : FORWARD;
    m->externaldir = (m->externaldir == FORWARD) ? REVERSE : FORWARD;
    adjust_pc();

    status = step_processor(-1);
    if (status != EXEC_FINISH) {
        fprintf(stderr,
                "check_reversible: reverse run did not reach START "
                "(step_processor returned %d)\n", status);
        return 1;
    }

    snapshot_take(&after, mem_max);

    if (snapshot_eq(&before, &after)) {
        printf("check_reversible: %s OK\n", input);
        return 0;
    } else {
        fprintf(stderr, "check_reversible: %s DIVERGED\n", input);
        return 1;
    }
}
