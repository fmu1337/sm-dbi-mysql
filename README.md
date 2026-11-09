# sm-dbi-mysql

A MySQL driver for SourceMod's DBI (`dbi.mysql.ext.so`) for servers whose SourceMod
is too old to talk to **MySQL 8.4+ / Oracle HeatWave** (`caching_sha2_password`, TLS 1.2/1.3).
It replaces the stock `mysql` driver of an old SourceMod. This repository is kept as a public
archive for those builds. Newer SourceMod versions ship this driver themselves.

- Driver source: AlliedModders `extensions/mysql` at commit
  [`b34de51`](https://github.com/alliedmodders/sourcemod/tree/b34de51b8c7f92fc2a7242fc0195e2d64a69cefd),
  including the locking fixes for [SourceMod #1207](https://github.com/alliedmodders/sourcemod/issues/1207)
  (upstream #2528) and #2554.
- Built from source: MariaDB Connector/C 3.4.9 (upstream v3.4.9 commit cc4d78c, static) and OpenSSL 3.0.22 (static), 32-bit Linux, glibc from Ubuntu 20.04. Nothing is prebuilt.
- `caching_sha2_password`, `sha256_password`, `mysql_native_password`; default charset `utf8mb4`.
- `ssl_mode` and `ssl_ca` in `databases.cfg` (see below).

## Compatibility

| SourceMod | DBI | File | Status |
|-----------|-----|------|--------|
| 1.5.x  | v9  | `dbi.mysql.sm15.ext.so`  | built only |
| 1.6.x  | v9  | `dbi.mysql.sm16.ext.so`  | built only |
| 1.7.x  | v9  | `dbi.mysql.sm17.ext.so`  | run on a real server: CS:S v34, SourceMod 1.7.3, Oracle HeatWave over TLS, `sm_mysql_hw_test` (2026-11-09); `sm_mysql_hw_ssl` not run there |
| 1.8.x  | v9  | `dbi.mysql.sm18.ext.so`  | built only |
| 1.9.x  | v9  | `dbi.mysql.sm19.ext.so`  | built only |
| 1.10.x | v9  | `dbi.mysql.sm110.ext.so` | built only |
| 1.11.x | v9  | `dbi.mysql.sm111.ext.so` | built only |
| 1.12.x | v9  | `dbi.mysql.sm112.ext.so` | built only |
| 1.13 (upstream master) | v10 | `dbi.mysql.sm113.ext.so` | built only |

Status meanings:

- **tested on a real server**: loaded by SourceMod on a game server and run through `sm_mysql_hw_test` and `sm_mysql_hw_ssl`.
- **smoke-tested**: loaded in CI and the test plugin run against MySQL 8.4. Not in v2.0.0 (see Known limits).
- **built only**: compiled in CI against the pinned SDK, with a published sha256. Not loaded.

The sm17 build has been in production on a CS:S v34 server (SourceMod 1.7.3, Oracle HeatWave over TLS) since
2026-11-09:

- 68 plugins load with no errors, including clientprefs, Shop and SourceBans++.
- `sm_mysql_hw_test` passes, including a server-side `KILL` of the shared connection while it is in use.
  With v1.0.0 this case crashed the server (SourceMod #1207).

That server runs a build of the v2.0.0 code from an earlier CI run (sha256 `e971c1fc…`). Builds are not
bit-reproducible. The other labels are built only.

SourceMod 1.5 to 1.12 use DBI interface v9; 1.13 and upstream master use v10. The source is the same for both.
The one DBI v10 addition, `DatabaseInfo::schemaName`, is not used by this driver. A binary is built for one
DBI version. Whether a binary built on an older SDK loads on a newer core is not tested.

## Install

1. Pick the file for your SourceMod version (table above). Check its sha256 against `sha256SUMS`.
2. Put it at `addons/sourcemod/extensions/dbi.mysql.ext.so`, replacing any other `dbi.mysql` binary.
3. Restart the server. `sm exts list` shows the build, for example `2.0.0-sm17 (MariaDB C/C 3.4.9, OpenSSL 3.0.22 ...)`.
4. Add `driver "mysql"` sections to `addons/sourcemod/configs/databases.cfg` (examples below).

## databases.cfg

```
"Databases"
{
	"default"
	{
		"driver"		"mysql"
		"host"			"db.example.com"
		"database"		"sourcemod"
		"user"			"sm"
		"pass"			"..."
		"port"			"3306"
		"timeout"		"10"
		"ssl_mode"		"verify_identity"
		"ssl_ca"		"configs/mysql-ca.pem"
	}
}
```

`ssl_mode` values, with the same names and meaning as the MySQL client options:

| `ssl_mode` | TLS | certificate chain checked | host name checked | needs `ssl_ca` |
|------------|-----|---------------------------|-------------------|----------------|
| `disabled` (default) | no | no | no | no |
| `preferred` | if the server offers it, otherwise plain | no | no | no |
| `required` | yes, or the connection fails | no | no | no |
| `verify_ca` | yes, or the connection fails | yes | no | yes |
| `verify_identity` | yes, or the connection fails | yes | yes | yes |

- `ssl_ca` is a PEM CA certificate. A relative path is taken from `addons/sourcemod/`.
- An unknown `ssl_mode`, or a verify mode without a readable `ssl_ca`, fails every connect with a message.
- Without `ssl_mode` the connection is plain, as in stock SourceMod. v1.0.0 defaulted to `preferred`; v2.0.0 defaults to `disabled`. Set `ssl_mode` explicitly.
- `verify_identity` needs `host` to match a name in the server certificate. An IP address fails unless the certificate lists it.
- After connect, `required` and the verify modes refuse a session that is not encrypted.
- Each connect logs one line, for example `connected host=... db=... ssl_mode=required tls=<cipher>`, or `tls=off`.

How `ssl_mode` reaches the driver: SourceMod core passes only the classic fields to a driver. The extension
therefore re-reads `configs/databases.cfg` with SourceMod's own SMC parser and matches the section by
`driver mysql`, `host`, `user`, `pass`, `database` and `port`. Two matching sections that differ in `ssl_mode` or
`ssl_ca` are an error. Give them a different host, user, database or port. `SQL_ConnectCustom()` has no section,
so it is plain.

A persistent connection (`SQL_Connect` with a name) is shared only with sections that have the same `ssl_mode` and `ssl_ca`.

## Test plugin

`scripting/mysql_heatwave_test.sp` (artifact `mysql_heatwave_test.smx`). It covers: one persistent handle shared by
two configs; threaded and synchronous `SQL_Query` on that handle; a killed connection; utf8mb4; prepared statements;
transactions; and the ssl_mode matrix.

```
sm_mysql_hw_test              # PASS/FAIL per case, then a SUMMARY line
sm_mysql_hw_ssl mysql_ssl     # tests mysql_ssl_disabled ... mysql_ssl_verify_identity
```

Sections used by the plugin (names as in the source):

```
"mysql_test_a"              { driver mysql, host H, user U, pass P, database T, port 3306 }
"mysql_test_b"              { driver mysql, host H, user U, pass P, database T, port 3306 }
"mysql_test_killer"         { driver mysql, host H, user U, pass P, database information_schema, port 3306 }
"mysql_ssl_disabled"        { ... ssl_mode disabled }
"mysql_ssl_preferred"       { ... ssl_mode preferred }
"mysql_ssl_required"        { ... ssl_mode required }
"mysql_ssl_verify_ca"       { ... ssl_mode verify_ca, ssl_ca configs/mysql-ca.pem }
"mysql_ssl_verify_identity" { ... ssl_mode verify_identity, ssl_ca configs/mysql-ca.pem }
```

Run the ssl matrix against a TLS server and against a server without TLS (second prefix, for example `mysql_ssl_notls`).
Expected: `disabled` gives an empty cipher; `preferred` gives a cipher on the TLS server and none on the other;
`required` and the verify modes fail on the server without TLS.

## Build

Linux with 32-bit multilib gcc. CI uses Ubuntu 20.04.

```bash
SMSDK=/path/to/sourcemod ./scripts/build-label.sh sm17
```

`scripts/build-label.sh <label>` fetches the SDK, metamod headers, MariaDB Connector/C and OpenSSL at the commits and
hashes in `matrix.txt` and `versions.env`, builds, checks that no TLS or MySQL symbol is exported, and writes
`dist/<label>/`. CI runs it for every label (`.github/workflows/build.yml`).

Layout:

- `vendor/sourcemod-mysql/`: upstream `extensions/mysql` at the pin, unmodified (see `vendor/VENDOR.md`).
- `overlay/`: this project's changes. Each file replaces the vendored file of the same name.
- `scripts/prepare.sh`: assembles `work-<label>/` from `vendor/`, `overlay/` and the SDK's `smsdk_ext.cpp`.

## Known limits

- `ssl_mode` is read from `databases.cfg`. A `SQL_ConnectCustom()` connection cannot use it.
- Sections that differ only in `ssl_mode` or `ssl_ca` need a different host, user, database or port.
- The TLS calls follow the css34 SourceMod fork's 1.13 driver. They are not re-verified in this repository.
- Of v2.0.0, only the sm17 build has run on a game server (see Compatibility). Every other label is built only.
- CI logs in to a MySQL 8.4 service container as a `caching_sha2_password` user, over TLS and without TLS, using the static client library. It does not load the extension in a game server yet. Planned: SteamCMD srcds and SourceMod release drops.
- Not checked: whether one binary built on the oldest SDK loads on a newer core. Ship the per-label build.
- Windows is not built.

## Upgrade from v1.0.0

- v1.0.0 worked only because it paired the prebuilt AlliedModders MariaDB Connector/C 3.4.9-sm.5 library (built against OpenSSL 1.1) with Ubuntu 20.04's OpenSSL 1.1.1f. v2.0.0 builds MariaDB Connector/C from source against OpenSSL 3.0.22 and checks every binary for unresolved symbols (`scripts/check-binary.sh`).
- **v1.0.0 has the race in [SourceMod #1207](https://github.com/alliedmodders/sourcemod/issues/1207).** A synchronous
  `SQL_Query` on the main thread and a threaded query can use the same persistent connection at once. Both can close
  TLS after a read error on a dead connection. v2.0.0 has the upstream locking fix. Replace v1.0.0 binaries.
- The `ssl_mode` default changed from `preferred` to `disabled`. Set `ssl_mode` explicitly if your configs relied on the default.
- `ssl_mode preferred` falls back to plain text on a server without TLS. v1.0.0 refused the connection there.
- `verify_ca` no longer checks the host name. v1.0.0 checked it for both verify modes.

## Licenses

This project's code and the vendored SourceMod source: GPL-3.0 (`LICENSE`).
MariaDB Connector/C (static): LGPL-2.1. OpenSSL (static): Apache-2.0. See `THIRD_PARTY.md`.
