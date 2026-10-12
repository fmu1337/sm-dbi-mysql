# sm-dbi-mysql

Drop-in `dbi.mysql.ext.so` for SourceMod on **CS:S v34 (Linux i386)** with support for **Oracle MySQL HeatWave / MySQL 8.4+**:

- `caching_sha2_password` / `sha256_password` / `mysql_native_password`
- TLS 1.2/1.3 via statically linked OpenSSL
- Default charset `utf8mb4`
- Optional `ssl_mode` / `ssl_ca` in `databases.cfg` (re-parsed by the extension; SM core ignores unknown keys)

Built against [MariaDB Connector/C 3.4.9](https://github.com/alliedmodders/mariadb-connector-c/releases/tag/v3.4.9-sm.5) (same client AlliedModders switched to on SourceMod master / 1.13 nightlies).

## Artifacts

| File | Target |
|------|--------|
| `dbi.mysql.sm17.ext.so` | SourceMod 1.7.x (fun / gg) |
| `dbi.mysql.sm18.ext.so` | SourceMod 1.8.x (Zombie Riot) |
| `dbi.mysql.sm111.ext.so` | SourceMod 1.11.x (DM prod) |
| `dbi.mysql.sm113.ext.so` | SourceMod 1.13 css34 fork (`fmu1337/sourcemod-css34`) |

Rename to `dbi.mysql.ext.so` and place in `addons/sourcemod/extensions/`.

## databases.cfg

```
"default"
{
	"driver"			"mysql"
	"host"				"10.0.0.81"
	"database"			"sourcemod"
	"user"				"sm"
	"pass"				"..."
	"port"				"3306"
	"timeout"			"10"
	"ssl_mode"			"preferred"   // disabled | preferred | required | verify_ca | verify_identity
	// "ssl_ca"			"configs/mysql-ca.pem"   // path relative to addons/sourcemod/
}
```

Defaults: `ssl_mode=preferred`. Without the new keys the driver behaves like a modern MariaDB/MySQL client (TLS preferred, no cert verify).

**Note:** SourceMod core only forwards `driver/host/database/user/pass/timeout/port` into the DBI driver. This extension reads `ssl_*` by re-parsing `configs/databases.cfg` and matching host+user+database(+port).

## ProxySQL rollback

Until servers are switched, ProxySQL on `micro-ams-1:6033` still accepts `mysql_native_password` without TLS and talks to HeatWave with sha2+TLS. To migrate a server: install the matching `.so`, point `host`/`port` at HeatWave (`10.0.0.81:3306`), set `ssl_mode` as needed. To roll back: restore the old extension binary and ProxySQL host/port.

## Test plugin

`scripting/mysql_heatwave_test.sp` — compile with the server’s spcomp, load, run:

```
sm_mysql_hw_test
```

Uses the `default` database section.

## Build (CI)

GitHub Actions builds all four variants in `ubuntu:20.04` with `-m32`, static MariaDB Connector/C + OpenSSL, symbol version script (`GetSMExtAPI` only). See `.github/workflows/build.yml`.

Local (Linux):

```bash
export SMSDK=/path/to/sourcemod MMSOURCE=/path/to/mmsource
export MARIADB=/path/to/mariadb-connector-c-3.4.9-x86
./scripts/prepare.sh
cd work && make SM_LABEL=sm17
```

## License

GPLv3 (SourceMod). MariaDB Connector/C is LGPL 2.1.
