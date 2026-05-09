#!/usr/bin/env bash
set -u

OS="$(uname -s 2>/dev/null || echo unknown)"
case "$OS" in
    Linux)
        ;;
    *)
        echo "skip"
        exit 0
        ;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SO_PATH="$REPO_ROOT/bin/libzmalloc_preload.so"

if [ ! -f "$SO_PATH" ]; then
    echo "[preload_regression] missing $SO_PATH"
    exit 2
fi

if ! command -v timeout >/dev/null 2>&1; then
    echo "[preload_regression] timeout not found, skip"
    exit 0
fi

TIMEOUT_SEC=30

run_cmd()
{
    local desc="$1"
    shift
    env -i PATH=/usr/bin:/bin LD_PRELOAD="$SO_PATH" timeout "$TIMEOUT_SEC" "$@" >/dev/null 2>&1
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "[preload_regression] FAIL ($rc): $desc"
        return 1
    fi
    return 0
}

run_pipe_cmd()
{
    local desc="$1"
    shift
    env -i PATH=/usr/bin:/bin LD_PRELOAD="$SO_PATH" timeout "$TIMEOUT_SEC" bash -c "$*" >/dev/null 2>&1
    local rc=$?
    if [ $rc -ne 0 ]; then
        echo "[preload_regression] FAIL ($rc): $desc"
        return 1
    fi
    return 0
}

run_cmd "/bin/true" /bin/true || exit 1
run_cmd "/bin/echo hello" /bin/echo hello || exit 1
run_cmd "/bin/ls -la /tmp" /bin/ls -la /tmp || exit 1
run_cmd "/bin/cat /etc/hostname" /bin/cat /etc/hostname || exit 1
run_cmd "/bin/sh -c 'echo from-sh'" /bin/sh -c 'echo from-sh' || exit 1
run_pipe_cmd "/bin/grep -r preload <repo>/src --include=*.h | head -n 20" \
    "/bin/grep -r preload \"$REPO_ROOT/src\" --include='*.h' | head -n 20" || exit 1

echo "[preload_regression] all commands ok"
exit 0
