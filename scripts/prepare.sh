#!/usr/bin/env bash
# Assemble build tree: SMSDK mysql sources + our overlay.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SMSDK="${SMSDK:?set SMSDK to SourceMod checkout}"
OUT="${OUT:-$ROOT/work}"

rm -rf "$OUT"
mkdir -p "$OUT/sdk" "$OUT/mysql"

cp -a "$SMSDK/extensions/mysql/mysql/"*.cpp "$SMSDK/extensions/mysql/mysql/"*.h "$OUT/mysql/"
cp -a "$SMSDK/extensions/mysql/extension.h" "$OUT/" 2>/dev/null || true
cp -a "$SMSDK/public/smsdk_ext.cpp" "$OUT/sdk/smsdk_ext.cpp"

# Overlay HeatWave / MariaDB patches
cp -a "$ROOT/overlay/extension.cpp" "$OUT/extension.cpp"
cp -a "$ROOT/overlay/extension.h" "$OUT/extension.h"
cp -a "$ROOT/overlay/smsdk_config.h" "$OUT/smsdk_config.h"
cp -a "$ROOT/overlay/mysql/MyDriver.cpp" "$OUT/mysql/MyDriver.cpp"
cp -a "$ROOT/overlay/mysql/MyDriver.h" "$OUT/mysql/MyDriver.h"
cp -a "$ROOT/overlay/mysql/MysqlSslConfig.cpp" "$OUT/mysql/MysqlSslConfig.cpp"
cp -a "$ROOT/overlay/mysql/MysqlSslConfig.h" "$OUT/mysql/MysqlSslConfig.h"

cp -a "$ROOT/Makefile" "$OUT/Makefile"
cp -a "$ROOT/exports.map" "$OUT/exports.map"

# Ensure smsdk_config is picked up from local dir (smsdk_ext.h includes it)
# Older trees expect smsdk_config.h next to extension sources.
echo "Prepared $OUT from SMSDK=$SMSDK"
