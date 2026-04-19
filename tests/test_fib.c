/* test_fib.c - whole-program check for the bundled fib.pisa.
 *
 * fib.pisa is a reversible iterative Fibonacci routine that computes
 * fib(18) = 2584 and copies the result into $6. The DATA cell n_value
 * is loaded into $5 via EXCH, so this test also exercises Haulund's
 * DATA directive end-to-end. check_reversible covers the round-trip;
 * this test nails down the specific forward-run value and the
 * scratch-register hygiene. */

#include <stdio.h>
#include "test_support.h"

extern char *progname;

void setUp(void)    { }
void tearDown(void) { }

static void test_fib18_produces_2584(void)
{
    int status;

    init_machine();
    load_imem("fib.pisa");

    status = step_processor(-1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(EXEC_FINISH, status,
        "fib.pisa did not reach FINISH");

    /* fib(18) = 2584. The result is copied from $1 into $6 just
       before FINISH; $1 still holds fib(18) and $2 holds fib(17). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2584, m->reg[6], "$6 != fib(18)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2584, m->reg[1], "$1 != fib(18)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1597, m->reg[2], "$2 != fib(17)");

    /* Loop index is left at N. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(18, m->reg[3], "$3 != N at exit");

    /* Scratch registers used for the DATA round-trip must come back to
       zero before FINISH. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[4],
        "$4 (DATA address scratch) not cleared before FINISH");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, m->reg[5],
        "$5 (N loaded from DATA) not cleared before FINISH");
}

int main(void)
{
    progname = "test_fib";
    UNITY_BEGIN();
    RUN_TEST(test_fib18_produces_2584);
    return UNITY_END();
}
