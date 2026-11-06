#!/usr/bin/env bash
# Gate for a built extension: fails on any unresolved symbol and on any library
# other than the system runtime, then dlopen()s it the way srcds does.
#
#   scripts/check-binary.sh dist/sm17/dbi.mysql.sm17.ext.so
set -euo pipefail

BIN="${1:?usage: check-binary.sh <extension.so>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# 1. Every undefined symbol must resolve against the libraries it is linked to.
#    (ldd -r performs the relocations and reports what is still missing.)
relocs="$(ldd -r "$BIN" 2>&1 || true)"
if echo "$relocs" | grep -q "undefined symbol"; then
	echo "$relocs" | grep "undefined symbol" >&2
	echo "ERROR: unresolved symbols in $BIN" >&2
	exit 1
fi

# 2. Only system runtime libraries may be needed (no OpenSSL, MariaDB, or other .so).
allowed='^(linux-gate\.so\.1|linux-vdso\.so\.1|libc\.so\.6|libm\.so\.6|libdl\.so\.2|libpthread\.so\.0|librt\.so\.1|libz\.so\.1|libstdc\+\+\.so\.6|libgcc_s\.so\.1|ld-linux\.so\.2)$'
bad="$(ldd "$BIN" | awk '{print $1}' | grep -Ev "$allowed" | grep -v '^$' || true)"
if [ -n "$bad" ]; then
	echo "ERROR: unexpected shared library dependencies:" >&2
	echo "$bad" >&2
	exit 1
fi

# 3. dlopen with RTLD_NOW, as srcds does.
work="$(mktemp -d)"
cc -m32 -o "$work/dlopen-check" "$ROOT/scripts/dlopen-check.c" -ldl
"$work/dlopen-check" "$BIN"
rm -rf "$work"
echo "check-binary OK: $BIN"
