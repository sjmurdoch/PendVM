/* test_branch.c - branches accumulate into BR rather than writing PC directly.
 *
 * RBRA additionally flips m->dir.
 * START halts only when m->dir == FORWARD.
 * FINISH halts only when m->dir == REVERSE.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- simple BR accumulation --- */

static void test_bra_adds_to_BR(void)
{
    m->BR = 0;
    i_bra(17, 0, 0);
    TEST_ASSERT_EQUAL_INT(17, (int)m->BR);
    i_bra(3, 0, 0);
    TEST_ASSERT_EQUAL_INT(20, (int)m->BR);
}

/* --- conditional branches: fire when condition true --- */

static void test_beq_fires_when_equal(void)
{
    m->reg[1] = 5;
    m->reg[2] = 5;
    m->BR = 0;
    i_beq(1, 2, 42);
    TEST_ASSERT_EQUAL_INT(42, (int)m->BR);
}

static void test_beq_noop_when_not_equal(void)
{
    m->reg[1] = 5;
    m->reg[2] = 6;
    m->BR = 0;
    i_beq(1, 2, 42);
    TEST_ASSERT_EQUAL_INT(0, (int)m->BR);
}

static void test_bne_fires_when_not_equal(void)
{
    m->reg[1] = 5;
    m->reg[2] = 6;
    i_bne(1, 2, 7);
    TEST_ASSERT_EQUAL_INT(7, (int)m->BR);
}

static void test_bgez_fires_on_zero(void)
{
    m->reg[1] = 0;
    i_bgez(1, 3, 0);
    TEST_ASSERT_EQUAL_INT(3, (int)m->BR);
}

static void test_bltz_fires_on_negative(void)
{
    m->reg[1] = -1;
    i_bltz(1, 9, 0);
    TEST_ASSERT_EQUAL_INT(9, (int)m->BR);
}

static void test_bltz_skips_on_zero(void)
{
    m->reg[1] = 0;
    i_bltz(1, 9, 0);
    TEST_ASSERT_EQUAL_INT(0, (int)m->BR);
}

static void test_bgtz_fires_on_positive(void)
{
    m->reg[1] = 5;
    i_bgtz(1, 11, 0);
    TEST_ASSERT_EQUAL_INT(11, (int)m->BR);
}

static void test_blez_fires_on_zero_and_negative(void)
{
    m->reg[1] = 0;
    i_blez(1, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, (int)m->BR);

    m->BR = 0;
    m->reg[1] = -7;
    i_blez(1, 4, 0);
    TEST_ASSERT_EQUAL_INT(4, (int)m->BR);
}

/* --- RBRA flips direction --- */

static void test_rbra_flips_direction_and_adds_BR(void)
{
    m->BR = 0;
    m->dir = FORWARD;
    i_rbra(100, 0, 0);
    TEST_ASSERT_EQUAL_INT(100, (int)m->BR);
    TEST_ASSERT_EQUAL_INT(REVERSE, m->dir);

    i_rbra(50, 0, 0);
    TEST_ASSERT_EQUAL_INT(150, (int)m->BR);
    TEST_ASSERT_EQUAL_INT(FORWARD, m->dir);
}

/* --- START / FINISH direction-gated halt --- */

static void test_start_halts_only_forward(void)
{
    m->dir = FORWARD;
    TEST_ASSERT_EQUAL_INT(0, i_start(0, 0, 0));  /* 0 = execute, continue */
    m->dir = REVERSE;
    TEST_ASSERT_EQUAL_INT(-3, i_start(0, 0, 0)); /* -3 = halt */
}

static void test_finish_halts_only_reverse(void)
{
    m->dir = REVERSE;
    TEST_ASSERT_EQUAL_INT(0, i_finish(0, 0, 0));
    m->dir = FORWARD;
    TEST_ASSERT_EQUAL_INT(-3, i_finish(0, 0, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bra_adds_to_BR);
    RUN_TEST(test_beq_fires_when_equal);
    RUN_TEST(test_beq_noop_when_not_equal);
    RUN_TEST(test_bne_fires_when_not_equal);
    RUN_TEST(test_bgez_fires_on_zero);
    RUN_TEST(test_bltz_fires_on_negative);
    RUN_TEST(test_bltz_skips_on_zero);
    RUN_TEST(test_bgtz_fires_on_positive);
    RUN_TEST(test_blez_fires_on_zero_and_negative);
    RUN_TEST(test_rbra_flips_direction_and_adds_BR);
    RUN_TEST(test_start_halts_only_forward);
    RUN_TEST(test_finish_halts_only_reverse);
    return UNITY_END();
}
