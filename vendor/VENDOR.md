# Vendored source

`vendor/sourcemod-mysql/` is a verbatim copy of upstream
`extensions/mysql/` (files `extension.cpp`, `extension.h`, `smsdk_config.h`,
`mysql/*.cpp`, `mysql/*.h`) from

- repository: https://github.com/alliedmodders/sourcemod
- commit: `b34de51b8c7f92fc2a7242fc0195e2d64a69cefd` (tree of SourceMod 1.13 master, 2026-10)
- license: GPL-3.0 (see `LICENSE` at the repository root)

It includes the upstream MySQL locking fixes (#2528 "Harden MySQL locking",
which fixes SourceMod bug #1207) and #2554.

Not vendored: `extensions/mysql/AMBuilder`, `msvc15hack.c`, `version.rc`
(AMBuild and Windows-only); this project builds with its own Makefile.

Changes made by this project live in `overlay/` and replace the files of the
same name when `scripts/prepare.sh` runs. Nothing under `vendor/` is edited.
To update the pin, replace `vendor/sourcemod-mysql/` from the new commit, record
the new commit here, and rebuild all labels.

## Checksums (sha256 of the vendored files at the pin)

a4352115d217220761b1dfa0e56c3e04b9b1a67f92a86fb30216c0ebdaf1c76a *./extension.cpp
1e9127970e9bf434e53507e38e56b19d1aab2522fb00019feb21fb26807725a7 *./extension.h
a18ca8e66bede9ad91de47fde65f51d6184e41b53b96833675d997ffbee20459 *./mysql/MyBasicResults.cpp
c97dee6297e38be3681879f7e196d1676021b0436fc2eef1cd3abda44f01bce7 *./mysql/MyBasicResults.h
c5ee188ab8dc4ed21430c0242261d3cab0e5d0d9a1718145e42d34a0ea292cd0 *./mysql/MyBoundResults.cpp
aa54374ba3eb11094e31ed54def241d855e9d1291dfe16c7356fc6bec550f07b *./mysql/MyBoundResults.h
a5aea96a1a2a01bd3d2bdb6ae9cd8aee866d2ff17e1a46ab173346baea468f7c *./mysql/MyDatabase.cpp
5b247c80920cd7508f3ca21cba563ba24280b6483815e04fdf66cd6abf2e09ff *./mysql/MyDatabase.h
e2c8c74a24699e9091dfcdc82cd85b30ccd4019674c5b6ca85016c79b39efbee *./mysql/MyDriver.cpp
cf10d379621bc1cb5b87dcc29c3a7245d530bdc42046fc8eb921a422bf73a649 *./mysql/MyDriver.h
33ffea1bff325b8f0f74a726be7d852aff6752a3ccf6fb344c909d993b984a09 *./mysql/MyStatement.cpp
42d3a63c95c53230f64219bbd505cfea7544e898b4cac0b87f0fcb8175ed2a7c *./mysql/MyStatement.h
09b1bb3a8def06048ee00b4a4401cdb3518aceee62a9ef322cd7362465b2e6b2 *./smsdk_config.h
