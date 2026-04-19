#!/bin/sh
# run_integration.sh - invoke check_reversible on every tests/programs/*.pisa
#
# Convention: programs named "break*" are expected to FAIL the reversibility
# check (they exist to prove the harness catches irreversible runs). All
# other programs are expected to pass.

set -u
status=0
count=0
for prog in tests/programs/*.pisa; do
    [ -f "$prog" ] || continue
    count=$((count + 1))
    name=$(basename "$prog" .pisa)
    echo "=== $prog ==="
    case "$name" in
        break*)
            if ./tests/check_reversible "$prog" 2>&1; then
                echo "FAIL: $prog was expected to diverge but passed"
                status=1
            else
                echo "OK (expected divergence)"
            fi
            ;;
        *)
            if ./tests/check_reversible "$prog" 2>&1; then
                echo "OK"
            else
                echo "FAIL: $prog did not round-trip"
                status=1
            fi
            ;;
    esac
done

if [ "$count" -eq 0 ]; then
    echo "run_integration.sh: no programs to run"
    exit 1
fi

# Also exercise the bundled fib.pisa at the project root — the canonical
# "./pendvm fib.pisa" smoke test. Rewriting it in Haulund dialect (see
# docs/PISA_COMPLIANCE_PLAN.md §7 step 5) means check_reversible should
# round-trip it.
echo "=== fib.pisa ==="
if ./tests/check_reversible fib.pisa 2>&1; then
    echo "OK"
else
    echo "FAIL: fib.pisa did not round-trip"
    status=1
fi

exit $status
