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

#include "MysqlSslConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "smsdk_ext.h"
#include <ma_pvio.h>

using namespace SourceMod;

/* ---------------------------------------------------------------------------
 * ssl_mode names
 * ------------------------------------------------------------------------- */

static const struct
{
	const char *name;
	MysqlSslMode mode;
} kModes[] = {
	{"disabled", MysqlSsl_Disabled},
	{"preferred", MysqlSsl_Preferred},
	{"required", MysqlSsl_Required},
	{"verify_ca", MysqlSsl_VerifyCA},
	{"verify_identity", MysqlSsl_VerifyIdentity},
};

static bool EqualsNoCase(const char *a, const char *b)
{
	for (;; a++, b++)
	{
		unsigned char x = (unsigned char)*a, y = (unsigned char)*b;
		if (x >= 'A' && x <= 'Z')
			x = x - 'A' + 'a';
		if (y >= 'A' && y <= 'Z')
			y = y - 'A' + 'a';
		if (x != y)
			return false;
		if (!x)
			return true;
	}
}

/* NULL or "" is the default (disabled). False for any other unknown value. */
static bool ParseMode(const char *value, MysqlSslMode *mode)
{
	if (!value || !value[0])
	{
		*mode = MysqlSsl_Disabled;
		return true;
	}
	for (size_t i = 0; i < sizeof(kModes) / sizeof(kModes[0]); i++)
	{
		if (EqualsNoCase(value, kModes[i].name))
		{
			*mode = kModes[i].mode;
			return true;
		}
	}
	return false;
}

const char *MysqlSslModeName(MysqlSslMode mode)
{
	for (size_t i = 0; i < sizeof(kModes) / sizeof(kModes[0]); i++)
	{
		if (kModes[i].mode == mode)
			return kModes[i].name;
	}
	return "?";
}

/* ---------------------------------------------------------------------------
 * databases.cfg
 *
 * SourceMod core passes a DatabaseInfo without the ssl_* keys, so the sections are
 * read again with the same SMC parser core uses (textparsers, not a hand parser).
 * A file is read on every connect: connects are rare and this keeps edits live.
 * ------------------------------------------------------------------------- */

struct ConfigEntry
{
	std::string host;
	std::string user;
	std::string pass;
	std::string database;
	std::string driver;
	std::string ssl_mode;
	std::string ssl_ca;
	unsigned int port;
	bool hasSslMode;
	bool hasSslCa;

	ConfigEntry() : port(0), hasSslMode(false), hasSslCa(false) {}
};

class ConfigListener : public ITextListener_SMC
{
public:
	ConfigListener() : m_Depth(0) {}

	void ReadSMC_ParseStart() override
	{
		m_Depth = 0;
	}

	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name) override
	{
		(void)states;
		(void)name;
		m_Depth++;
		if (m_Depth == 2)
		{
			m_Current = ConfigEntry();
		}
		return SMCResult_Continue;
	}

	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value) override
	{
		(void)states;
		if (m_Depth == 1)
		{
			if (EqualsNoCase(key, "driver_default"))
				m_DriverDefault = value;
			return SMCResult_Continue;
		}
		if (m_Depth != 2)
			return SMCResult_Continue;

		if (EqualsNoCase(key, "driver"))
			m_Current.driver = value;
		else if (EqualsNoCase(key, "host"))
			m_Current.host = value;
		else if (EqualsNoCase(key, "user"))
			m_Current.user = value;
		else if (EqualsNoCase(key, "pass"))
			m_Current.pass = value;
		else if (EqualsNoCase(key, "database"))
			m_Current.database = value;
		else if (EqualsNoCase(key, "port"))
			m_Current.port = (unsigned int)strtoul(value, NULL, 10);
		else if (EqualsNoCase(key, "ssl_mode"))
		{
			m_Current.ssl_mode = value;
			m_Current.hasSslMode = true;
		}
		else if (EqualsNoCase(key, "ssl_ca"))
		{
			m_Current.ssl_ca = value;
			m_Current.hasSslCa = true;
		}
		return SMCResult_Continue;
	}

	SMCResult ReadSMC_LeavingSection(const SMCStates *states) override
	{
		(void)states;
		if (m_Depth == 2)
		{
			m_Entries.push_back(m_Current);
		}
		m_Depth--;
		return SMCResult_Continue;
	}

	/* An entry belongs to this driver when its driver is mysql, or when it names
	 * no driver and driver_default is mysql (SourceMod's own rule). */
	bool IsMysql(const ConfigEntry &e) const
	{
		if (EqualsNoCase(e.driver.c_str(), "mysql"))
			return true;
		if (e.driver.empty() || EqualsNoCase(e.driver.c_str(), "default"))
			return EqualsNoCase(m_DriverDefault.c_str(), "mysql");
		return false;
	}

	const std::vector<ConfigEntry> &Entries() const { return m_Entries; }

private:
	int m_Depth;
	ConfigEntry m_Current;
	std::string m_DriverDefault;
	std::vector<ConfigEntry> m_Entries;
};

static bool SameText(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
		return a == b;
	return strcmp(a, b) == 0;
}

static bool SameSsl(const ConfigEntry &a, const ConfigEntry &b)
{
	return a.hasSslMode == b.hasSslMode
		&& a.ssl_mode == b.ssl_mode
		&& a.hasSslCa == b.hasSslCa
		&& a.ssl_ca == b.ssl_ca;
}

/* Resolves ssl_ca against addons/sourcemod/ unless it is absolute. */
static bool ResolveCa(const char *ca, MysqlSslSettings *out, char *error, size_t maxlength)
{
	bool absolute = (ca[0] == '/' || ca[0] == '\\')
		|| (((ca[0] >= 'A' && ca[0] <= 'Z') || (ca[0] >= 'a' && ca[0] <= 'z')) && ca[1] == ':');

	int written;
	if (absolute)
		written = snprintf(out->ca, sizeof(out->ca), "%s", ca);
	else
		written = snprintf(out->ca, sizeof(out->ca), "%s/%s", smutils->GetSourceModPath(), ca);

	if (written < 0 || (size_t)written >= sizeof(out->ca))
	{
		snprintf(error, maxlength, "ssl_ca path too long: \"%s\"", ca);
		out->ca[0] = '\0';
		return false;
	}

	FILE *fp = fopen(out->ca, "rb");
	if (!fp)
	{
		snprintf(error, maxlength, "ssl_mode %s: cannot read ssl_ca \"%s\"",
			MysqlSslModeName(out->mode), out->ca);
		out->ca[0] = '\0';
		return false;
	}
	fclose(fp);
	return true;
}

bool MysqlSslLookup(const DatabaseInfo *info, MysqlSslSettings *out, char *error, size_t maxlength)
{
	out->mode = MysqlSsl_Disabled;
	out->ca[0] = '\0';
	error[0] = '\0';

	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_SM, path, sizeof(path), "configs/databases.cfg");

	ConfigListener listener;
	SMCError perr = textparsers->ParseFile_SMC(path, &listener, NULL);
	if (perr != SMCError_Okay)
	{
		const char *why = textparsers->GetSMCErrorString(perr);
		snprintf(error, maxlength, "cannot parse databases.cfg for ssl_mode: %s", why ? why : "unknown error");
		return false;
	}

	/* Match on the fields SourceMod itself passes in. Sections that differ only by
	 * ssl_* keys cannot be told apart, so they must not share a connection. */
	const ConfigEntry *found = NULL;
	const std::vector<ConfigEntry> &entries = listener.Entries();
	for (size_t i = 0; i < entries.size(); i++)
	{
		const ConfigEntry &e = entries[i];
		if (!listener.IsMysql(e))
			continue;
		if (!SameText(e.host.c_str(), info->host)
			|| !SameText(e.user.c_str(), info->user)
			|| !SameText(e.pass.c_str(), info->pass)
			|| !SameText(e.database.c_str(), info->database)
			|| e.port != info->port)
			continue;

		if (!found)
		{
			found = &e;
		}
		else if (!SameSsl(*found, e))
		{
			snprintf(error, maxlength,
				"databases.cfg has two mysql sections for %s/%s with different ssl_mode or ssl_ca; "
				"give them different host, user, database or port",
				info->host ? info->host : "", info->database ? info->database : "");
			return false;
		}
	}

	if (!found)
		return true; /* no section: plain connection, as stock SourceMod */

	if (!ParseMode(found->hasSslMode ? found->ssl_mode.c_str() : NULL, &out->mode))
	{
		snprintf(error, maxlength,
			"ssl_mode \"%s\" is not one of disabled, preferred, required, verify_ca, verify_identity",
			found->ssl_mode.c_str());
		return false;
	}

	if (out->mode != MysqlSsl_VerifyCA && out->mode != MysqlSsl_VerifyIdentity)
		return true; /* only the verify modes read ssl_ca */

	if (!found->hasSslCa || found->ssl_ca.empty())
	{
		snprintf(error, maxlength,
			"ssl_mode %s needs ssl_ca (the server's CA certificate, PEM, relative to addons/sourcemod/)",
			MysqlSslModeName(out->mode));
		return false;
	}

	return ResolveCa(found->ssl_ca.c_str(), out, error, maxlength);
}

/* ---------------------------------------------------------------------------
 * MariaDB Connector/C options
 *
 * The verification callback and the TLS version are MariaDB 3.4 options. The
 * MARIADB_TLS head used by the callback below is the one in ma_tls.h of the
 * pinned release (MARIADB_PACKAGE_VERSION_ID 30409); the build stops if the
 * library is changed.
 * ------------------------------------------------------------------------- */

extern "C" int ma_pvio_tls_verify_server_cert(void *ctls, unsigned int flags);

#if MARIADB_PACKAGE_VERSION_ID != 30409
#error "sm-dbi-mysql: re-check MysqlSslConfig.cpp against this MariaDB Connector/C (MARIADB_TLS head)"
#endif

/* The leading members of MARIADB_TLS (ma_tls.h). */
struct MysqlTlsHead
{
	void *data;
	MARIADB_PVIO *pvio;
};

/* "verify_ca": the chain is checked, the host name is not. */
static int VerifyChainOnly(void *ctls, unsigned int flags)
{
	return ma_pvio_tls_verify_server_cert(ctls, flags & ~MARIADB_TLS_VERIFY_HOST);
}

/* "required": TLS, but no certificate check. OpenSSL records chain problems during
 * the handshake; the connector would fail the login later unless they are cleared. */
static int AcceptUnverified(void *ctls, unsigned int flags)
{
	(void)flags;
	MysqlTlsHead *tls = (MysqlTlsHead *)ctls;
	if (!tls || !tls->pvio || tls->pvio->ctls != ctls || !tls->pvio->mysql)
		return 1;
	tls->pvio->mysql->net.tls_verify_status = 0;
	return 0;
}

void MysqlSslApply(MYSQL *mysql, const MysqlSslSettings &ssl)
{
	if (ssl.mode == MysqlSsl_Disabled)
	{
		/* No TLS option at all: the connection stays plain, as before SSL support. */
		my_bool verify = 0;
		mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char *)&verify);
		return;
	}

	/* preferred / required / verify_*: TLS is requested. Certificate verification is
	 * on only for required and above (MariaDB 3.3.10+ verifies by default). */
	my_bool enforce = 1;
	my_bool verify = (ssl.mode >= MysqlSsl_Required);
	mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char *)&verify);
	mysql_options(mysql, MYSQL_OPT_SSL_ENFORCE, (const char *)&enforce);

	/* HeatWave and MySQL 8.4 speak TLS 1.2 and 1.3 only. */
	mysql_options(mysql, MARIADB_OPT_TLS_VERSION, "TLSv1.2,TLSv1.3");

	switch (ssl.mode)
	{
	case MysqlSsl_Required:
		mysql_options(mysql, MARIADB_OPT_TLS_VERIFICATION_CALLBACK, (const void *)AcceptUnverified);
		break;
	case MysqlSsl_VerifyCA:
		mysql_options(mysql, MYSQL_OPT_SSL_CA, ssl.ca);
		mysql_options(mysql, MARIADB_OPT_TLS_VERIFICATION_CALLBACK, (const void *)VerifyChainOnly);
		break;
	case MysqlSsl_VerifyIdentity:
		mysql_options(mysql, MYSQL_OPT_SSL_CA, ssl.ca);
		break;
	default:
		break;
	}
}

bool MysqlSslVerifySession(MYSQL *mysql, const MysqlSslSettings &ssl, char *error, size_t maxlength)
{
	/* preferred may fall back to plain; required and above must be encrypted. */
	if (ssl.mode >= MysqlSsl_Required && mysql_get_ssl_cipher(mysql) == NULL)
	{
		snprintf(error, maxlength, "[0]: ssl_mode %s: the session is not encrypted",
			MysqlSslModeName(ssl.mode));
		return false;
	}
	return true;
}

void MysqlSslDescribe(MYSQL *mysql, char *out, size_t maxlength)
{
	const char *cipher = mysql_get_ssl_cipher(mysql);
	if (cipher && cipher[0])
		snprintf(out, maxlength, "tls=%s", cipher);
	else
		snprintf(out, maxlength, "tls=off");
}
