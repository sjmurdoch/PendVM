/* test_logic.c - xor-writeback logical instructions.
 *
 * These instructions all compute some f(rs,rt) and XOR it into rd — so calling
 * them twice (even forward twice) is the identity. Tests verify both the
 * forward semantics and the self-inverse property.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- forward semantics: rd initially zero, so rd == f(rs,rt) --- */

static void test_andx_forward_xors_and_into_rd(void)
{
    m->reg[1] = 0xF0F0F0F0u;
    m->reg[2] = 0x0FFFF0F0u;
    i_andx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(0x00F0F0F0u, (unsigned)m->reg[3]);
}

static void test_orx_forward_xors_or_into_rd(void)
{
    m->reg[1] = 0x0000FF00u;
    m->reg[2] = 0x00FF0000u;
    i_orx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(0x00FFFF00u, (unsigned)m->reg[3]);
}

static void test_xorx_forward(void)
{
    /* i_xorx is 2-arg (rsd, rt): rsd ^= rt */
    m->reg[1] = 0xAAAAAAAAu;
    m->reg[2] = 0x55555555u;
    i_xorx(1, 2, 0);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, (unsigned)m->reg[1]);
}

static void test_norx_forward(void)
{
    m->reg[1] = 0x0000FF00u;
    m->reg[2] = 0x00FF0000u;
    i_norx(3, 1, 2);
    TEST_ASSERT_EQUAL_HEX32(~0x00FFFF00u, (unsigned)m->reg[3]);
}

static void test_andix_forward(void)
{
    m->reg[1] = 0xABCDEF01u;
    i_andix(3, 1, 0x0000FFFFu);
    TEST_ASSERT_EQUAL_HEX32(0x0000EF01u, (unsigned)m->reg[3]);
}

static void test_orix_forward(void)
{
    m->reg[1] = 0x00000F0Fu;
    i_orix(3, 1, 0xF000F000u);
    TEST_ASSERT_EQUAL_HEX32(0xF0000F0Fu | 0xF000F000u, (unsigned)m->reg[3]);
}

static void test_xorix_forward(void)
{
    m->reg[1] = 0x000000FFu;
    i_xorix(1, 0xAA, 0);
    TEST_ASSERT_EQUAL_HEX32(0x00000055u, (unsigned)m->reg[1]);
}

static void test_sltx_takes_less_than(void)
{
    m->reg[3] = 0;
    m->reg[1] = 1;
    m->reg[2] = 5;
    i_sltx(3, 1, 2);
    TEST_ASSERT_EQUAL_INT(1, m->reg[3]);
}

static void test_sltx_false_when_not_less(void)
{
    m->reg[3] = 0;
    m->reg[1] = 9;
    m->reg[2] = 2;
    i_sltx(3, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, m->reg[3]);
}

static void test_sltix_immediate(void)
{
    m->reg[3] = 0;
    m->reg[1] = 3;
    i_sltix(3, 1, 10);
    TEST_ASSERT_EQUAL_INT(1, m->reg[3]);
}

/* --- self-inverse property (call twice == identity) --- */

#define SELF_INVERSE_TEST(name, call)                  \
    static void test_ ## name ## _is_self_inverse(void) \
    {                                                   \
        int before[MAX_REG], after[MAX_REG];            \
        m->reg[1] = 0xDEADBEEFu;                        \
        m->reg[2] = 0xCAFEBABEu;                        \
        m->reg[3] = 0x12345678u;                        \
        test_snapshot_regs(before);                     \
        call; call;                                     \
        test_snapshot_regs(after);                      \
        test_assert_regs_eq(before, after);             \
    }

SELF_INVERSE_TEST(andx,  i_andx(3, 1, 2))
SELF_INVERSE_TEST(orx,   i_orx(3, 1, 2))
SELF_INVERSE_TEST(xorx,  i_xorx(1, 2, 0))
SELF_INVERSE_TEST(norx,  i_norx(3, 1, 2))
SELF_INVERSE_TEST(andix, i_andix(3, 1, 0x0000FFFFu))
SELF_INVERSE_TEST(orix,  i_orix(3, 1, 0xF000F000u))
SELF_INVERSE_TEST(xorix, i_xorix(1, 0xAA, 0))
SELF_INVERSE_TEST(sltx,  i_sltx(3, 1, 2))
SELF_INVERSE_TEST(sltix, i_sltix(3, 1, 10))

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_andx_forward_xors_and_into_rd);
    RUN_TEST(test_orx_forward_xors_or_into_rd);
    RUN_TEST(test_xorx_forward);
    RUN_TEST(test_norx_forward);
    RUN_TEST(test_andix_forward);
    RUN_TEST(test_orix_forward);
    RUN_TEST(test_xorix_forward);
    RUN_TEST(test_sltx_takes_less_than);
    RUN_TEST(test_sltx_false_when_not_less);
    RUN_TEST(test_sltix_immediate);
    RUN_TEST(test_andx_is_self_inverse);
    RUN_TEST(test_orx_is_self_inverse);
    RUN_TEST(test_xorx_is_self_inverse);
    RUN_TEST(test_norx_is_self_inverse);
    RUN_TEST(test_andix_is_self_inverse);
    RUN_TEST(test_orix_is_self_inverse);
    RUN_TEST(test_xorix_is_self_inverse);
    RUN_TEST(test_sltx_is_self_inverse);
    RUN_TEST(test_sltix_is_self_inverse);
    return UNITY_END();
}
