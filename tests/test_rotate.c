/* test_rotate.c - RL / RR / RLV / RRV reversibility.
 *
 * Rotate instructions are NOT self-inverse: they actually rotate rsd
 * destructively. Reversibility comes from the direction flag: in REVERSE,
 * RL rotates right (and RR rotates left). Tests verify the round-trip.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- forward rotate semantics --- */

static void test_rl_forward_rotates_left(void)
{
    /* 0x80000001 << 1 | 0x80000001 >> 31 = 0x00000003 */
    m->reg[1] = 0x80000001u;
    i_rl(1, 1, 0);
    TEST_ASSERT_EQUAL_HEX32(0x00000003u, (unsigned)m->reg[1]);
}

static void test_rr_forward_rotates_right(void)
{
    m->reg[1] = 0x00000003u;
    i_rr(1, 1, 0);
    TEST_ASSERT_EQUAL_HEX32(0x80000001u, (unsigned)m->reg[1]);
}

static void test_rl_zero_amount_is_noop(void)
{
    m->reg[1] = 0x12345678u;
    i_rl(1, 0, 0);
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, (unsigned)m->reg[1]);
}

/* --- direction-flipped round trip --- */

static void test_rl_then_reverse_rl_restores(void)
{
    int before[MAX_REG];
    m->reg[1] = 0xCAFEBABEu;
    test_snapshot_regs(before);

    i_rl(1, 7, 0);
    m->dir = REVERSE;
    i_rl(1, 7, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

static void test_rr_then_reverse_rr_restores(void)
{
    int before[MAX_REG];
    m->reg[1] = 0xDEADBEEFu;
    test_snapshot_regs(before);

    i_rr(1, 11, 0);
    m->dir = REVERSE;
    i_rr(1, 11, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

static void test_rlv_round_trip(void)
{
    int before[MAX_REG];
    m->reg[1] = 0xABCD1234u;
    m->reg[2] = 5;                    /* rotate count */
    test_snapshot_regs(before);

    i_rlv(1, 2, 0);
    m->dir = REVERSE;
    i_rlv(1, 2, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

static void test_rrv_round_trip(void)
{
    int before[MAX_REG];
    m->reg[1] = 0x13579BDFu;
    m->reg[2] = 13;
    test_snapshot_regs(before);

    i_rrv(1, 2, 0);
    m->dir = REVERSE;
    i_rrv(1, 2, 0);
    m->dir = FORWARD;

    {
        int after[MAX_REG];
        test_snapshot_regs(after);
        test_assert_regs_eq(before, after);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rl_forward_rotates_left);
    RUN_TEST(test_rr_forward_rotates_right);
    RUN_TEST(test_rl_zero_amount_is_noop);
    RUN_TEST(test_rl_then_reverse_rl_restores);
    RUN_TEST(test_rr_then_reverse_rr_restores);
    RUN_TEST(test_rlv_round_trip);
    RUN_TEST(test_rrv_round_trip);
    return UNITY_END();
}
