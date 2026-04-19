/* test_mult.c - whole-program check for Frank Figure 8-2's multiplication
 * routine (Reversibility for Efficient Computing, pp. 213-214). The routine
 * plus its driver live at tests/programs/mult_frank.pisa; this test loads
 * the program, runs it forward to FINISH, and asserts:
 *
 *   - the product $5 matches m1*m2 for the driver's hardcoded 7 * 11,
 *   - the input operands $3 and $4 are preserved (Frank's grade-school
 *     shift-and-add uncomputes everything except the product),
 *   - every auxiliary register used by the routine ($2/$28-$31) is
 *     restored to zero — "aux regs clean on return",
 *   - the stack pointer $1 has returned to its base.
 *
 * The paired check that the program is reversible (forward -> reverse ->
 * initial state) is exercised by tests/check_reversible via the integration
 * harness, not here. */

#include <stdio.h>
#include "test_support.h"

extern char *progname;

void setUp(void)    { }
void tearDown(void) { }

static void test_mult_frank_produces_product(void)
{
    int status;

    init_machine();
    load_imem("tests/programs/mult_frank.pisa");

    status = step_processor(-1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXEC_FINISH, status,
        "mult driver did not reach FINISH");

    /* Product */
    TEST_ASSERT_EQUAL_INT_MESSAGE(77, m->reg[5], "$5 != 7 * 11");

    /* Operands preserved (grade-school is non-destructive for m1, m2). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(7,  m->reg[3], "$3 (m1) was clobbered");
    TEST_ASSERT_EQUAL_INT_MESSAGE(11, m->reg[4], "$4 (m2) was clobbered");

    /* Stack pointer returns to the driver's base value. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(200, m->reg[1], "$1 (SP) not restored");

    /* Auxiliary registers must all be zero on return. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[2],  "$2 (SRO/mask) not cleared");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[28], "$28 (bit) not cleared");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[29], "$29 (shifted) not cleared");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[30], "$30 (position) not cleared");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[31], "$31 (limit) not cleared");
}

int main(void)
{
    progname = "test_mult";
    UNITY_BEGIN();
    RUN_TEST(test_mult_frank_produces_product);
    return UNITY_END();
}
