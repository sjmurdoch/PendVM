/* test_parse.c - parse_reg / parse_immed dispatch and the endptr bug.
 */

#include "test_support.h"

void setUp(void)    { test_reset_machine(); }
void tearDown(void) { }

/* --- parse_reg --- */

static void test_parse_reg_valid(void)
{
    TEST_ASSERT_EQUAL_INT(0,  parse_reg("$0"));
    TEST_ASSERT_EQUAL_INT(5,  parse_reg("$5"));
    TEST_ASSERT_EQUAL_INT(31, parse_reg("$31"));
}

static void test_parse_reg_bad_format(void)
{
    TEST_ASSERT_EQUAL_INT(-1, parse_reg("5"));
    TEST_ASSERT_EQUAL_INT(-1, parse_reg("r5"));
}

static void test_parse_reg_out_of_range(void)
{
    TEST_ASSERT_EQUAL_INT(-2, parse_reg("$32"));
    TEST_ASSERT_EQUAL_INT(-2, parse_reg("$100"));
}

/* --- parse_immed happy-path --- */

static void test_parse_immed_decimal(void)
{
    WORD *v = parse_immed("42", 16);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_UINT(42u, *v);
}

static void test_parse_immed_hex(void)
{
    WORD *v = parse_immed("0x10", 16);
    TEST_ASSERT_NOT_NULL(v);
    TEST_ASSERT_EQUAL_UINT(16u, *v);
}

static void test_parse_immed_empty_returns_null(void)
{
    TEST_ASSERT_NULL(parse_immed("", 16));
    TEST_ASSERT_NULL(parse_immed(NULL, 16));
}

/* --- KNOWN BUG: parse_immed passes a NULL endptr to strtol, then checks
 * `if(endptr)` — which is the pointer-to-char**, not *endptr, and since it
 * starts as NULL the condition is always false and garbage is never
 * rejected. Pin the current behavior so anyone fixing it will see this
 * test break and update it alongside their patch.
 * See pal_parse.c:209-221. */
static void test_parse_immed_accepts_garbage_CURRENT_BEHAVIOR(void)
{
    WORD *v = parse_immed("12garbage", 16);
    TEST_ASSERT_NOT_NULL_MESSAGE(
        v,
        "KNOWN BUG (pal_parse.c:209-221): parse_immed should reject this, "
        "but the endptr check always compares the pointer itself, so garbage "
        "passes. Fix the bug AND update this test together.");
    TEST_ASSERT_EQUAL_UINT(12u, *v);
}

/* --- parse_inst dispatch: exercise happy path through the full parser --- */

static void test_parse_inst_dispatches_addi(void)
{
    char args[3][16];
    int status;

    m->reg[5] = 0;
    strcpy(args[0], "$5");
    strcpy(args[1], "99");
    args[2][0] = 0;

    status = parse_inst("", "ADDI", args);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_INT(99, m->reg[5]);
}

static void test_parse_inst_unknown_mnemonic(void)
{
    char args[3][16];
    int status;
    args[0][0] = args[1][0] = args[2][0] = 0;
    status = parse_inst("", "NOT_A_REAL_INSTRUCTION", args);
    TEST_ASSERT_EQUAL_INT(-1, status);
}

static void test_parse_inst_bad_register(void)
{
    char args[3][16];
    int status;
    strcpy(args[0], "$99");    /* out of range */
    strcpy(args[1], "1");
    args[2][0] = 0;
    status = parse_inst("", "ADDI", args);
    TEST_ASSERT_EQUAL_INT(-1, status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_reg_valid);
    RUN_TEST(test_parse_reg_bad_format);
    RUN_TEST(test_parse_reg_out_of_range);
    RUN_TEST(test_parse_immed_decimal);
    RUN_TEST(test_parse_immed_hex);
    RUN_TEST(test_parse_immed_empty_returns_null);
    RUN_TEST(test_parse_immed_accepts_garbage_CURRENT_BEHAVIOR);
    RUN_TEST(test_parse_inst_dispatches_addi);
    RUN_TEST(test_parse_inst_unknown_mnemonic);
    RUN_TEST(test_parse_inst_bad_register);
    return UNITY_END();
}
