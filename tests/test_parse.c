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

/* Malformed literals like "12garbage" or "abc" must be rejected. (Plan §3.4.) */
static void test_parse_immed_rejects_garbage(void)
{
    TEST_ASSERT_NULL(parse_immed("12garbage", 16));
    TEST_ASSERT_NULL(parse_immed("abc", 16));
    TEST_ASSERT_NULL(parse_immed("0xFFzz", 16));
}

/* Immediates that exceed the signed len-bit range are rejected. (Plan §3.3.) */
static void test_parse_immed_rejects_out_of_range(void)
{
    TEST_ASSERT_NULL(parse_immed("32768", 16));      /* just past signed 16 max */
    TEST_ASSERT_NULL(parse_immed("-32769", 16));     /* just past signed 16 min */
    TEST_ASSERT_NOT_NULL(parse_immed("32767", 16));  /* signed 16 max, ok */
    TEST_ASSERT_NOT_NULL(parse_immed("-32768", 16)); /* signed 16 min, ok */
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

/* After plan §3.3 the AMT position must reject values outside 0..31. */
static void test_parse_inst_rejects_oversized_amt(void)
{
    char args[3][16];
    strcpy(args[0], "$3");
    strcpy(args[1], "$1");
    strcpy(args[2], "32"); /* one past the 5-bit max */
    TEST_ASSERT_EQUAL_INT(-1, parse_inst("", "SLLX", args));

    strcpy(args[2], "-1");
    TEST_ASSERT_EQUAL_INT(-1, parse_inst("", "SLLX", args));

    strcpy(args[2], "31"); /* max valid */
    m->reg[1] = 1;
    m->reg[3] = 0;
    TEST_ASSERT_EQUAL_INT(0, parse_inst("", "SLLX", args));
}

/* After plan §3.3 + §3.4 the IMM position must reject 16-bit overflow. */
static void test_parse_inst_rejects_imm_overflow(void)
{
    char args[3][16];
    strcpy(args[0], "$5");
    strcpy(args[1], "32768"); /* one past signed 16-bit max */
    args[2][0] = 0;
    TEST_ASSERT_EQUAL_INT(-1, parse_inst("", "ADDI", args));

    strcpy(args[1], "-32769"); /* one past signed 16-bit min */
    TEST_ASSERT_EQUAL_INT(-1, parse_inst("", "ADDI", args));
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
    RUN_TEST(test_parse_immed_rejects_garbage);
    RUN_TEST(test_parse_immed_rejects_out_of_range);
    RUN_TEST(test_parse_inst_dispatches_addi);
    RUN_TEST(test_parse_inst_unknown_mnemonic);
    RUN_TEST(test_parse_inst_bad_register);
    RUN_TEST(test_parse_inst_rejects_oversized_amt);
    RUN_TEST(test_parse_inst_rejects_imm_overflow);
    return UNITY_END();
}
