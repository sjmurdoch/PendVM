/* test_shift.c - SLLX/SRLX/SRAX (fixed-amount) and SLLVX/SRLVX/SRAVX
 * (reg-amount) reversibility and semantics.
 *
 * All shifts use XOR writeback into rd, so they are self-inverse: calling
 * twice with the same operands restores the original rd.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- forward semantics (rd initially 0, so rd == shifted value) --- */

static void test_sllx_fixed(void)
{
    m->reg[1] = 0x1u;
    i_sllx(3, 1, 4);
    TEST_ASSERT_EQUAL_HEX32(0x10u, (unsigned)m->reg[3]);
}

static void test_srlx_fixed(void)
{
    m->reg[1] = 0x100u;
    i_srlx(3, 1, 4);
    TEST_ASSERT_EQUAL_HEX32(0x10u, (unsigned)m->reg[3]);
}

static void test_srlx_actually_arithmetic_CURRENT_BEHAVIOR(void)
{
    /* BUG (pinned): i_srlx does `m->reg[rs] >> amt` where reg[] is signed int,
     * so it performs an ARITHMETIC shift, not the logical shift the mnemonic
     * implies. SRAX and SRLX currently produce the same result on negative
     * inputs. Change this test only if the VM's shift semantics are fixed. */
    m->reg[1] = 0x80000000u;
    i_srlx(3, 1, 4);
    TEST_ASSERT_EQUAL_HEX32(0xF8000000u, (unsigned)m->reg[3]);
}

static void test_srax_sign_extends_negative(void)
{
    /* SRAX should sign-fill the high bits for negative inputs. */
    m->reg[1] = 0x80000000u;
    i_srax(3, 1, 4);
    TEST_ASSERT_EQUAL_HEX32(0xF8000000u, (unsigned)m->reg[3]);
}

static void test_sllvx_reg_amount(void)
{
    m->reg[1] = 0x1u;
    m->reg[2] = 8;
    i_sllvx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(0x100u, (unsigned)m->reg[3]);
}

static void test_srlvx_reg_amount(void)
{
    m->reg[1] = 0xFF00u;
    m->reg[2] = 4;
    i_srlvx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(0x0FF0u, (unsigned)m->reg[3]);
}

static void test_sravx_sign_extends(void)
{
    m->reg[1] = 0x80000000u;
    m->reg[2] = 4;
    i_sravx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(0xF8000000u, (unsigned)m->reg[3]);
}

/* --- self-inverse --- */

#define SELF_INVERSE_TEST(name, call)                   \
    static void test_ ## name ## _self_inverse(void)     \
    {                                                    \
        int before[MAX_REG], after[MAX_REG];             \
        m->reg[1] = 0xDEADBEEFu;                         \
        m->reg[2] = 5;                                   \
        m->reg[3] = 0x5A5A5A5Au;                         \
        test_snapshot_regs(before);                      \
        call; call;                                      \
        test_snapshot_regs(after);                       \
        test_assert_regs_eq(before, after);              \
    }

SELF_INVERSE_TEST(sllx,   i_sllx(3, 1, 7))
SELF_INVERSE_TEST(srlx,   i_srlx(3, 1, 7))
SELF_INVERSE_TEST(srax,   i_srax(3, 1, 7))
SELF_INVERSE_TEST(sllvx,  i_sllvx(3, 1, 2))
SELF_INVERSE_TEST(srlvx,  i_srlvx(3, 1, 2))
SELF_INVERSE_TEST(sravx,  i_sravx(3, 1, 2))

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sllx_fixed);
    RUN_TEST(test_srlx_fixed);
    RUN_TEST(test_srlx_actually_arithmetic_CURRENT_BEHAVIOR);
    RUN_TEST(test_srax_sign_extends_negative);
    RUN_TEST(test_sllvx_reg_amount);
    RUN_TEST(test_srlvx_reg_amount);
    RUN_TEST(test_sravx_sign_extends);
    RUN_TEST(test_sllx_self_inverse);
    RUN_TEST(test_srlx_self_inverse);
    RUN_TEST(test_srax_self_inverse);
    RUN_TEST(test_sllvx_self_inverse);
    RUN_TEST(test_srlvx_self_inverse);
    RUN_TEST(test_sravx_self_inverse);
    return UNITY_END();
}
