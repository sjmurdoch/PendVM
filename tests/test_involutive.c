/* test_involutive.c - NEG / EXCH / SWAPBR. Each is its own inverse,
 * regardless of direction. Calling twice restores state.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- NEG --- */

static void test_neg_negates(void)
{
    m->reg[1] = 42;
    i_neg(1, 0, 0);
    TEST_ASSERT_EQUAL_INT(-42, m->reg[1]);
}

static void test_neg_is_involutive(void)
{
    int before[MAX_REG], after[MAX_REG];
    m->reg[1] = -12345;
    test_snapshot_regs(before);
    i_neg(1, 0, 0);
    i_neg(1, 0, 0);
    test_snapshot_regs(after);
    test_assert_regs_eq(before, after);
}

/* --- EXCH: swap register with data memory --- */

static void test_exch_swaps_reg_and_mem(void)
{
    MEMORY *cell = mem_get(0x100);
    cell->type = MEM_DATA;
    cell->value = 0xAAAA5555u;

    m->reg[1] = 0x12345678u;
    m->reg[2] = 0x100;    /* address */

    TEST_ASSERT_EQUAL_INT(0, i_exch(1, 2, 0));
    TEST_ASSERT_EQUAL_HEX32(0xAAAA5555u, (unsigned)m->reg[1]);
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, (unsigned)cell->value);
}

static void test_exch_is_involutive(void)
{
    MEMORY *cell = mem_get(0x200);
    WORD orig_mem;
    int orig_reg;

    cell->type = MEM_DATA;
    cell->value = 0xBEEFu;
    m->reg[1] = 0xCAFEu;
    m->reg[2] = 0x200;

    orig_reg = m->reg[1];
    orig_mem = cell->value;

    i_exch(1, 2, 0);
    i_exch(1, 2, 0);

    TEST_ASSERT_EQUAL_HEX32(orig_reg, (unsigned)m->reg[1]);
    TEST_ASSERT_EQUAL_HEX32(orig_mem, (unsigned)cell->value);
}

static void test_exch_refuses_instruction_memory(void)
{
    MEMORY *cell = mem_get(0x300);
    cell->type = MEM_INST;
    m->reg[1] = 0;
    m->reg[2] = 0x300;
    TEST_ASSERT_EQUAL_INT(-4, i_exch(1, 2, 0));
}

/* --- SWAPBR: exchange direction-normalized BR with a register --- */

static void test_swapbr_forward(void)
{
    m->BR = 0x10u;
    m->reg[1] = 0x40u;
    m->dir = FORWARD;
    i_swapbr(1, 0, 0);
    TEST_ASSERT_EQUAL_HEX32(0x40u, m->BR);
    TEST_ASSERT_EQUAL_HEX32(0x10u, (unsigned)m->reg[1]);
}

static void test_swapbr_is_involutive(void)
{
    WORD orig_BR;
    int orig_r;
    m->BR = 0x12345678u;
    m->reg[1] = 0x87654321u;
    orig_BR = m->BR;
    orig_r = m->reg[1];

    i_swapbr(1, 0, 0);
    i_swapbr(1, 0, 0);

    TEST_ASSERT_EQUAL_HEX32(orig_BR, m->BR);
    TEST_ASSERT_EQUAL_HEX32(orig_r, (unsigned)m->reg[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_neg_negates);
    RUN_TEST(test_neg_is_involutive);
    RUN_TEST(test_exch_swaps_reg_and_mem);
    RUN_TEST(test_exch_is_involutive);
    RUN_TEST(test_exch_refuses_instruction_memory);
    RUN_TEST(test_swapbr_forward);
    RUN_TEST(test_swapbr_is_involutive);
    return UNITY_END();
}
