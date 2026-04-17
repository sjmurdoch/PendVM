#!/bin/sh
# run_unit.sh - run every tests/test_* binary, OR their exit codes.

set -u
status=0
count=0
for bin in tests/test_*; do
    case "$bin" in
        *.c|*.h|*.sh|*.dSYM) continue ;;
    esac
    [ -f "$bin" ] || continue
    [ -x "$bin" ] || continue
    count=$((count + 1))
    echo "=== $bin ==="
    if ! "$bin"; then
        status=1
    fi
done

if [ "$count" -eq 0 ]; then
    echo "run_unit.sh: no test binaries found"
    exit 1
fi

exit $status
