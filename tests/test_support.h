/* test_support.h - shared helpers for PendVM unit tests.
 *
 * Test binaries link memory.o / machine.o / pal_parse.o / commands.o plus
 * a specially-compiled main_for_tests.o (main.c with main() renamed). That
 * gives us real EXTRACT/power/sign_extend and the progname/output_radix
 * globals without duplicating code or modifying main.c.
 */

#ifndef PENDVM_TEST_SUPPORT_H
#define PENDVM_TEST_SUPPORT_H

#include <string.h>
#include "unity.h"
#include "pendvm.h"

extern MACHINE *m;

/* Reset the MACHINE to a known state. mirrors init_machine() but callable
 * repeatedly from tests without leaking a new malloc each invocation. */
static void test_reset_machine(void)
{
    int i;
    if (!m) init_machine();
    m->PC = 0;
    m->BR = 0;
    m->dir = FORWARD;
    m->externaldir = FORWARD;
    m->reset = TRUE;
    m->time = 0;
    for (i = 0; i < MAX_REG; i++) m->reg[i] = 0;
}

/* Assert two snapshots of the GPR file are identical. */
static void test_assert_regs_eq(const int *a, const int *b)
{
    int i;
    for (i = 0; i < MAX_REG; i++) {
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(a[i], b[i], "register file diverged");
    }
}

/* Snapshot current GPRs into `out`. */
static void test_snapshot_regs(int *out)
{
    int i;
    for (i = 0; i < MAX_REG; i++) out[i] = m->reg[i];
}

#endif /* PENDVM_TEST_SUPPORT_H */
