# Third-party notices

The following are linked into `dbi.mysql.ext.so` or used to build it. Their licenses apply to their parts.

| Component | Version | License | Source |
|-----------|---------|---------|--------|
| MariaDB Connector/C (`libmariadbclient.a`, static) | 3.4.9 (AlliedModders release `v3.4.9-sm.5`) | LGPL-2.1 | https://github.com/alliedmodders/mariadb-connector-c/releases/tag/v3.4.9-sm.5 |
| OpenSSL (`libssl.a`, `libcrypto.a`, static) | 3.0.22 | Apache-2.0 | https://github.com/openssl/openssl/releases/tag/openssl-3.0.22 |
| SourceMod `extensions/mysql` (vendored source) | commit `b34de51b8c7f92fc2a7242fc0195e2d64a69cefd` | GPL-3.0 | https://github.com/alliedmodders/sourcemod |
| SourceMod SDK headers (build only) | per label, see `matrix.txt` | GPL-3.0 | https://github.com/alliedmodders/sourcemod |
| Metamod:Source `sourcehook` headers (build only) | commit `80e8ff0be3b62386bbd6f937e97b819ef8be6dd2` | GPL-3.0 | https://github.com/alliedmodders/metamod-source |

MariaDB Connector/C is used under LGPL-2.1. Its source is in the release above. The build links it as a static
library, and `MARIADB` in the `Makefile` selects which copy is used. OpenSSL is used under Apache-2.0; its
license and notices come with the OpenSSL source tree.

Copyright notices for the SourceMod code are in the vendored files under `vendor/sourcemod-mysql/`.
