#!/usr/bin/env bash
# Assemble the build tree work/: pinned upstream sources (vendor/) + this project's
# overlay/ (overlay wins), plus the SDK's smsdk_ext.cpp for the chosen SourceMod.
#
#   SMSDK=/path/to/sourcemod OUT=work ./scripts/prepare.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SMSDK="${SMSDK:?set SMSDK to a SourceMod SDK checkout}"
OUT="${OUT:-$ROOT/work}"

rm -rf "$OUT"
mkdir -p "$OUT/sdk" "$OUT/mysql"

# 1. Pinned upstream extensions/mysql (see vendor/VENDOR.md).
cp -a "$ROOT/vendor/sourcemod-mysql/." "$OUT/"

# 2. This project's changes; each file replaces the vendored one of the same name.
cp -a "$ROOT/overlay/." "$OUT/"

# 3. The SDK's own extension glue (SDK version differs per label).
# SourceMod 1.6+ ships public/smsdk_ext.{cpp,h}; 1.5 keeps them in an extension.
if [ -f "$SMSDK/public/smsdk_ext.cpp" ]; then
  glue="$SMSDK/public"
else
  glue="$(dirname "$(ls "$SMSDK"/extensions/*/sdk/smsdk_ext.cpp | head -1)")"
fi
# SourceMod 1.5 glue defines ITextParsers *textparsers twice (under the same #if); keep the first.
awk -f "$ROOT/scripts/dedupe-glue.awk" "$glue/smsdk_ext.cpp" > "$OUT/sdk/smsdk_ext.cpp"
cp -a "$glue/smsdk_ext.h" "$OUT/sdk/"

cp -a "$ROOT/Makefile" "$OUT/Makefile"
cp -a "$ROOT/exports.map" "$OUT/exports.map"

echo "Prepared $OUT from SMSDK=$SMSDK"
