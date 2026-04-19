/* test_arith.c - i_add / i_addi / i_sub reversibility and semantics. */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- semantics --- */

static void test_add_forward(void)
{
    m->reg[1] = 10;
    m->reg[2] = 7;
    TEST_ASSERT_EQUAL_INT(0, i_add(1, 2, 0));
    TEST_ASSERT_EQUAL_INT(17, m->reg[1]);
    TEST_ASSERT_EQUAL_INT(7,  m->reg[2]); /* rt unchanged */
}

static void test_addi_forward(void)
{
    m->reg[3] = 100;
    TEST_ASSERT_EQUAL_INT(0, i_addi(3, 42, 0));
    TEST_ASSERT_EQUAL_INT(142, m->reg[3]);
}

static void test_sub_forward(void)
{
    m->reg[4] = 20;
    m->reg[5] = 3;
    TEST_ASSERT_EQUAL_INT(0, i_sub(4, 5, 0));
    TEST_ASSERT_EQUAL_INT(17, m->reg[4]);
}

/* --- reversibility: dir-multiplied inverse --- */

static void test_add_reverse_undoes(void)
{
    int before[MAX_REG];
    m->reg[1] = 42;
    m->reg[2] = -13;
    test_snapshot_regs(before);

    i_add(1, 2, 0);
    m->dir = REVERSE;
    i_add(1, 2, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

static void test_addi_reverse_undoes(void)
{
    int before[MAX_REG];
    m->reg[3] = 0x1234;
    test_snapshot_regs(before);

    i_addi(3, 0xFF, 0);
    m->dir = REVERSE;
    i_addi(3, 0xFF, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

static void test_sub_reverse_undoes(void)
{
    int before[MAX_REG];
    m->reg[4] = 999;
    m->reg[5] = 77;
    test_snapshot_regs(before);

    i_sub(4, 5, 0);
    m->dir = REVERSE;
    i_sub(4, 5, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

/* reverse direction: i_add should subtract (since reg += dir*rt, dir=-1) */
static void test_add_in_reverse_dir(void)
{
    m->reg[1] = 50;
    m->reg[2] = 8;
    m->dir = REVERSE;
    i_add(1, 2, 0);
    TEST_ASSERT_EQUAL_INT(42, m->reg[1]);
}

/* Frank's inversion rule (Appendix B.3): ADD and SUB are paired inverses
   — performing ADD then SUB with the same operands is the identity in
   the forward direction. This is the compile-time pairing rule the
   compiler will rely on. */
static void test_add_then_sub_is_identity(void)
{
    int before[MAX_REG], after[MAX_REG];
    m->reg[1] = 0x1234;
    m->reg[2] = 0x7F;
    test_snapshot_regs(before);
    i_add(1, 2, 0);
    i_sub(1, 2, 0);
    test_snapshot_regs(after);
    test_assert_regs_eq(before, after);
}

/* ADDI is self-inverting when negated (Frank B.3: ADDI r c ↔ ADDI r -c). */
static void test_addi_plus_negated_is_identity(void)
{
    int before[MAX_REG], after[MAX_REG];
    m->reg[3] = 0xABCD;
    test_snapshot_regs(before);
    i_addi(3,  1234, 0);
    i_addi(3, -1234, 0);
    test_snapshot_regs(after);
    test_assert_regs_eq(before, after);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_add_forward);
    RUN_TEST(test_addi_forward);
    RUN_TEST(test_sub_forward);
    RUN_TEST(test_add_reverse_undoes);
    RUN_TEST(test_addi_reverse_undoes);
    RUN_TEST(test_sub_reverse_undoes);
    RUN_TEST(test_add_in_reverse_dir);
    RUN_TEST(test_add_then_sub_is_identity);
    RUN_TEST(test_addi_plus_negated_is_identity);
    return UNITY_END();
}
