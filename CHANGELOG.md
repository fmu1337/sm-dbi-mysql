# Changelog

## 2.0.0

- Driver source is the vendored AlliedModders `extensions/mysql` at `b34de51`, including the #1207 locking fix (upstream #2528) and #2554. v1.0.0 used copies of the 1.7-dev, 1.8-dev and 1.11-dev branch heads.
- One source tree for every SourceMod label: sm15, sm16, sm17, sm18, sm19, sm110, sm111, sm112 (DBI v9) and sm113 (DBI v10). Built from pinned SDK commits in `matrix.txt`.
- `ssl_mode` and `ssl_ca` follow the MySQL client modes (disabled, preferred, required, verify_ca, verify_identity), with the TLS calls of the css34 SourceMod fork's 1.13 driver. The default is now `disabled`.
- `ssl_mode` is read with SourceMod's SMC parser.
- Persistent connections are shared only between configs with the same TLS settings.
- utf8mb4 is requested at connect.
- The version string lists the build label, MariaDB Connector/C and OpenSSL. OpenSSL is 3.0.22 (v1.0.0 used 1.1.1f).
- Each connect logs the TLS cipher, or `tls=off`.
- The test plugin covers shared and threaded handles, a killed connection, utf8mb4, prepared statements, transactions and the ssl_mode matrix.
- Build inputs are pinned by commit or sha256. CI builds every label on Ubuntu 20.04.

## 1.0.0

- First release (SourceMod 1.7, 1.8, 1.11 and 1.13-fork builds). Contains the race in SourceMod #1207. Do not use it for new deployments.
