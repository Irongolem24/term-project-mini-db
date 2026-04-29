#!/usr/bin/env bash
# Parser-level integration tests for CREATE TABLE
set -euo pipefail

BINARY="../minidb_debug"
PASS=0
FAIL=0

check() {
    local desc="$1"
    local input="$2"
    local expected="$3"

    actual=$(printf "%s\n" "$input" | "$BINARY" 2>/dev/null | grep -v "^db> $" | grep -v "^db> " | tr -d '\r' || true)
    # strip prompt prefix if still present
    actual=$(printf "%s\n" "$input" | "$BINARY" 2>/dev/null | sed 's/^db> //' | grep -v '^$' | tr -d '\r' || true)

    if [ "$actual" = "$expected" ]; then
        echo "[PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "[FAIL] $desc"
        echo "       expected: $expected"
        echo "       actual:   $actual"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "=== Parser Integration Tests: CREATE TABLE ==="

check \
    "valid CREATE TABLE (INT PK + TEXT)" \
    "CREATE TABLE users (id INT PRIMARY KEY, name TEXT)" \
    "Success"

check \
    "valid CREATE TABLE (single INT column, no PK)" \
    "CREATE TABLE t (x INT)" \
    "Success"

check \
    "valid CREATE TABLE (FLOAT column)" \
    "CREATE TABLE scores (score FLOAT)" \
    "Success"

check \
    "duplicate table returns error" \
    "$(printf 'CREATE TABLE users (id INT PRIMARY KEY, name TEXT)\nCREATE TABLE users (id INT)')" \
    "$(printf 'Success\nerror: table '"'"'users'"'"' already exists')"

check \
    "unknown column type returns error" \
    "CREATE TABLE t (x BLOB)" \
    "error: unknown type 'BLOB'"

check \
    "missing table name (no paren group) returns syntax error" \
    "CREATE TABLE (id INT)" \
    "error: syntax is CREATE TABLE <name> (col TYPE [PRIMARY KEY], ...)"

check \
    "empty column list returns error" \
    "CREATE TABLE t ()" \
    "error: no columns defined"

check \
    "missing TABLE keyword returns syntax error" \
    "CREATE users (id INT)" \
    "error: syntax is CREATE TABLE <name> (col TYPE [PRIMARY KEY], ...)"

echo ""
echo "=== Result: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]
