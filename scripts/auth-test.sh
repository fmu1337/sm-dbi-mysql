#!/usr/bin/env bash
# Builds tests/auth/auth_test.c statically against the MariaDB Connector/C and OpenSSL
# of one build, and runs it against a MySQL server (CI: a MySQL 8.4 service container).
#
#   scripts/auth-test.sh <mariadb-prefix> <openssl-prefix> <host> <port> <root-password>
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MARIADB="${1:?mariadb prefix}"
OPENSSL="${2:?openssl prefix}"
HOST="${3:?host}"
PORT="${4:?port}"
ROOT_PW="${5:?root password}"

work="$(mktemp -d)"
cc -m32 -O1 -Wall -I"$MARIADB/include/mariadb" -o "$work/auth_test" "$ROOT/tests/auth/auth_test.c" \
	"$MARIADB/lib/libmariadbclient.a" "$OPENSSL/lib/libssl.a" "$OPENSSL/lib/libcrypto.a" \
	-lz -lpthread -ldl -lm
# No plugin directory exists here on purpose: every auth plugin must be inside the library.
"$work/auth_test" "$HOST" "$PORT" "$ROOT_PW"
rm -rf "$work"
