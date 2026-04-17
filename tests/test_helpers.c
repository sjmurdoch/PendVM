/* test_helpers.c - sanity tests for power / EXTRACT / sign_extend. */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

static void test_power_basics(void)
{
    TEST_ASSERT_EQUAL_UINT(1, power(2, 0));
    TEST_ASSERT_EQUAL_UINT(2, power(2, 1));
    TEST_ASSERT_EQUAL_UINT(1024, power(2, 10));
    TEST_ASSERT_EQUAL_UINT(1, power(7, 0));
    TEST_ASSERT_EQUAL_UINT(81, power(3, 4));
}

static void test_EXTRACT_bitfield(void)
{
    /* low..high inclusive, shifted to bit 0 */
    TEST_ASSERT_EQUAL_HEX32(0x0Fu, EXTRACT(0xDEADBEEFu, 0, 3));
    TEST_ASSERT_EQUAL_HEX32(0xEu,  EXTRACT(0xDEADBEEFu, 4, 7));
    TEST_ASSERT_EQUAL_HEX32(0xDu,  EXTRACT(0xDEADBEEFu, 28, 31));
    /* whole word */
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFu, EXTRACT(0xDEADBEEFu, 0, 31));
}

static void test_sign_extend_short_to_32(void)
{
    /* 16-bit value with high bit clear: unchanged */
    TEST_ASSERT_EQUAL_HEX32(0x00001234, sign_extend(0x1234, 16));

    /* 16-bit value with high bit set: should sign-extend to 0xFFFFnnnn */
    TEST_ASSERT_EQUAL_HEX32(0xFFFF8000u, (unsigned)sign_extend(0x8000, 16));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, (unsigned)sign_extend(0xFFFF, 16));

    /* length==32 passes value through verbatim */
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABEu, (unsigned)sign_extend(0xCAFEBABE, 32));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_basics);
    RUN_TEST(test_EXTRACT_bitfield);
    RUN_TEST(test_sign_extend_short_to_32);
    return UNITY_END();
}
