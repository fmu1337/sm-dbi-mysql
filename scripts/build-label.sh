#!/usr/bin/env bash
# Build dbi.mysql.<label>.ext.so for one SourceMod SDK label (sm15 ... sm113).
#
# Everything is fetched at the commit pinned in versions.env / matrix.txt and
# checked against its sha256 where a tarball is used. Output: dist/<label>/.
#
#   scripts/build-label.sh sm17
#
# Environment:
#   CACHE   download and build cache (default: $ROOT/.cache)
#   DIST    output directory (default: $ROOT/dist)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LABEL="${1:?usage: build-label.sh <label>, e.g. sm17}"
CACHE="${CACHE:-$ROOT/.cache}"
DIST="${DIST:-$ROOT/dist}"

# shellcheck disable=SC1091
. "$ROOT/versions.env"

read -r _label SM_VERSION SDK_COMMIT DBI_VERSION _ref < <(awk -v l="$LABEL" '$1 == l { $1=$1; print }' "$ROOT/matrix.txt")
if [ -z "${SDK_COMMIT:-}" ]; then
	echo "no matrix entry for $LABEL" >&2
	exit 1
fi
echo "== $LABEL: SourceMod $SM_VERSION (DBI v$DBI_VERSION) at $SDK_COMMIT"

mkdir -p "$CACHE" "$DIST/$LABEL"

verify_sha256() { # file sha
	echo "$2  $1" | sha256sum -c --status || { echo "sha256 mismatch: $1" >&2; exit 1; }
}

# Fetch one commit (shallow) into dir; submodules are pinned by that commit.
fetch_commit() { # repo commit dir [submodule paths...]
	local repo="$1" commit="$2" dir="$3"
	shift 3
	if [ ! -d "$dir/.git" ]; then
		git init -q "$dir"
		git -C "$dir" remote add origin "$repo"
	fi
	if [ "$(git -C "$dir" rev-parse -q --verify HEAD 2>/dev/null || true)" != "$commit" ]; then
		git -C "$dir" fetch -q --depth 1 origin "$commit"
		git -C "$dir" checkout -q --force --detach FETCH_HEAD
	fi
	# Old SDKs have no submodules (or vendor amtl): skip them there.
	if [ "$#" -gt 0 ] && [ -f "$dir/.gitmodules" ]; then
		git -C "$dir" submodule update -q --init --depth 1 -- "$@"
	fi
}

SDK="$CACHE/sdk-$LABEL"
fetch_commit "https://github.com/alliedmodders/sourcemod" "$SDK_COMMIT" "$SDK" public/amtl sourcepawn

MMSOURCE="$CACHE/mmsource"
fetch_commit "$MMSOURCE_REPO" "$MMSOURCE_COMMIT" "$MMSOURCE"

MARIADB="$CACHE/$MARIADB_DIR"
if [ ! -f "$MARIADB/lib/libmariadbclient.a" ]; then
	tarball="$CACHE/mariadb-$MARIADB_RELEASE.tar.gz"
	[ -f "$tarball" ] || curl -fsSL -o "$tarball" "$MARIADB_URL"
	verify_sha256 "$tarball" "$MARIADB_SHA256"
	tar -xzf "$tarball" -C "$CACHE"
fi

OPENSSL_PREFIX="$CACHE/openssl-$OPENSSL_VERSION-i386"
if [ ! -f "$OPENSSL_PREFIX/lib/libcrypto.a" ]; then
	tarball="$CACHE/openssl-$OPENSSL_VERSION.tar.gz"
	[ -f "$tarball" ] || curl -fsSL -o "$tarball" "$OPENSSL_URL"
	verify_sha256 "$tarball" "$OPENSSL_SHA256"
	src="$CACHE/openssl-src-$OPENSSL_VERSION-i386"
	rm -rf "$src"
	mkdir -p "$src"
	tar -xzf "$tarball" -C "$src" --strip-components=1
	(
		cd "$src"
		./Configure linux-generic32 -m32 -fPIC no-shared no-tests no-docs no-engine \
			--prefix="$OPENSSL_PREFIX" --openssldir="$OPENSSL_PREFIX/ssl"
		make -j"$(nproc)" build_libs
		make install_dev
	)
fi

WORK="$ROOT/work-$LABEL"
OUT="$WORK" SMSDK="$SDK" "$ROOT/scripts/prepare.sh"

make -C "$WORK" -j"$(nproc)" \
	SMSDK="$SDK" \
	MMSOURCE="$MMSOURCE" \
	MARIADB="$MARIADB" \
	OPENSSL="$OPENSSL_PREFIX" \
	SM_LABEL="$LABEL"

BUILT="$WORK/build/$LABEL/dbi.mysql.ext.so"
test -f "$BUILT"

echo "=== ldd ==="
ldd "$BUILT" || true
# No shared TLS or MySQL client libraries: everything is linked in statically.
if ldd "$BUILT" | grep -E 'libmysql|libmariadb|libssl\.so|libcrypto\.so'; then
	echo "ERROR: unexpected shared library dependency" >&2
	exit 1
fi
echo "=== exported symbols ==="
nm -D --defined-only "$BUILT" | head -50
if nm -D --defined-only "$BUILT" | grep -E ' (SSL_|EVP_|mysql_|ma_)'; then
	echo "ERROR: TLS or MySQL client symbols are exported" >&2
	exit 1
fi

cp "$BUILT" "$DIST/$LABEL/dbi.mysql.$LABEL.ext.so"
(cd "$DIST/$LABEL" && sha256sum "dbi.mysql.$LABEL.ext.so" > "dbi.mysql.$LABEL.ext.so.sha256")
cat "$DIST/$LABEL/dbi.mysql.$LABEL.ext.so.sha256"
