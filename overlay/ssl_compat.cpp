/**
 * sm-dbi-mysql v2: OpenSSL 3.0 symbol the prebuilt MariaDB Connector/C 3.4.9 needs.
 *
 * The MariaDB static library calls SSL_get_peer_certificate(). OpenSSL 1.1 exported
 * it as a function; OpenSSL 3.0 only has it as a deprecated macro for
 * SSL_get1_peer_certificate(). Nothing defined it, so the .so had an undefined
 * symbol that the linker does not report for a shared object, and dlopen() failed on
 * SourceMod 1.7 (`undefined symbol: SSL_get_peer_certificate`). This file defines it
 * with the same meaning (a new reference that the caller frees). The symbol stays
 * local to the extension (-fvisibility=hidden and exports.map).
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 */

#include <openssl/ssl.h>

#ifdef SSL_get_peer_certificate
#undef SSL_get_peer_certificate
#endif

extern "C" X509 *SSL_get_peer_certificate(const SSL *s)
{
	return SSL_get1_peer_certificate(s);
}
