/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * sm-dbi-mysql v2: ssl_mode / ssl_ca for the MySQL DBI driver.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INCLUDE_SM_MYSQL_SSL_CONFIG_H_
#define _INCLUDE_SM_MYSQL_SSL_CONFIG_H_

#include <IDBDriver.h>
#include <mysql.h>
#include <stddef.h>

/*
 * Two extra keys in addons/sourcemod/configs/databases.cfg, read by this
 * extension itself (SourceMod core forwards only the classic fields):
 *
 *   ssl_mode  disabled (default) | preferred | required | verify_ca | verify_identity
 *   ssl_ca    CA certificate (PEM) for verify_ca / verify_identity; relative paths are
 *             taken from addons/sourcemod/
 *
 * Semantics follow the MySQL client options and the css34 SourceMod fork's 1.13 driver:
 *
 *   disabled         no TLS is requested.
 *   preferred        TLS if the server offers it; the certificate is not checked.
 *   required         TLS or no connection; the certificate is not checked.
 *   verify_ca        TLS; the certificate chain must verify against ssl_ca; host name not checked.
 *   verify_identity  as verify_ca, and the host name must match the certificate.
 */

enum MysqlSslMode
{
	MysqlSsl_Disabled = 0,
	MysqlSsl_Preferred,
	MysqlSsl_Required,
	MysqlSsl_VerifyCA,
	MysqlSsl_VerifyIdentity
};

#define MYSQL_SSL_PATH_MAX 1024

struct MysqlSslSettings
{
	MysqlSslMode mode;
	char ca[MYSQL_SSL_PATH_MAX];

	MysqlSslSettings() : mode(MysqlSsl_Disabled) { ca[0] = '\0'; }
};

/* Name of a mode, lower case, as written in databases.cfg. */
const char *MysqlSslModeName(MysqlSslMode mode);

/*
 * Finds the ssl_mode / ssl_ca of the databases.cfg section that describes this
 * connection (driver mysql, same host, user, password, database and port).
 * Returns false with an error for the plugin when the section is malformed or
 * ambiguous (two matching sections with different ssl settings).
 */
bool MysqlSslLookup(const SourceMod::DatabaseInfo *info, MysqlSslSettings *out, char *error, size_t maxlength);

/* Sets the TLS options of a MYSQL handle before mysql_real_connect(). */
void MysqlSslApply(MYSQL *mysql, const MysqlSslSettings &ssl);

/* Checks the session after connect (required and up must be encrypted). */
bool MysqlSslVerifySession(MYSQL *mysql, const MysqlSslSettings &ssl, char *error, size_t maxlength);

/* One line for the SourceMod log: "tls=<cipher>" or "tls=off". */
void MysqlSslDescribe(MYSQL *mysql, char *out, size_t maxlength);

#endif /* _INCLUDE_SM_MYSQL_SSL_CONFIG_H_ */
