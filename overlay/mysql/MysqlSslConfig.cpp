/**
 * Parse ssl_mode / ssl_ca from databases.cfg (core drops unknown keys).
 */

#include "MysqlSslConfig.h"
#include "smsdk_ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

MysqlSslConfig g_MysqlSslConfig;

static MysqlSslMode ParseMode(const char *value)
{
	if (!value || !value[0])
		return MysqlSsl_Preferred;
	if (strcasecmp(value, "disabled") == 0)
		return MysqlSsl_Disabled;
	if (strcasecmp(value, "preferred") == 0)
		return MysqlSsl_Preferred;
	if (strcasecmp(value, "required") == 0)
		return MysqlSsl_Required;
	if (strcasecmp(value, "verify_ca") == 0)
		return MysqlSsl_VerifyCA;
	if (strcasecmp(value, "verify_identity") == 0)
		return MysqlSsl_VerifyIdentity;
	return MysqlSsl_Preferred;
}

static void TrimInPlace(char *s)
{
	char *start = s;
	while (*start && isspace((unsigned char)*start))
		start++;
	if (start != s)
		memmove(s, start, strlen(start) + 1);
	size_t len = strlen(s);
	while (len && isspace((unsigned char)s[len - 1]))
		s[--len] = '\0';
}

static void Unquote(char *s)
{
	TrimInPlace(s);
	size_t len = strlen(s);
	if (len >= 2 && s[0] == '"' && s[len - 1] == '"')
	{
		s[len - 1] = '\0';
		memmove(s, s + 1, len - 1);
	}
}

void MysqlSslConfig::Init()
{
	m_Count = 0;
	m_Loaded = false;
	m_Mtime = -1;
	m_Path[0] = '\0';
	if (smutils)
		smutils->BuildPath(Path_SM, m_Path, sizeof(m_Path), "configs/databases.cfg");
	Reload();
}

void MysqlSslConfig::Reload()
{
	if (!m_Path[0] && smutils)
		smutils->BuildPath(Path_SM, m_Path, sizeof(m_Path), "configs/databases.cfg");
	if (!m_Path[0])
		return;

	struct stat st;
	long long mtime = 0;
	if (stat(m_Path, &st) == 0)
		mtime = (long long)st.st_mtime;
	if (m_Loaded && mtime == m_Mtime)
		return;

	m_Count = 0;
	ParseFile(m_Path);
	m_Mtime = mtime;
	m_Loaded = true;
}

void MysqlSslConfig::ParseFile(const char *path)
{
	FILE *fp = fopen(path, "rt");
	if (!fp)
		return;

	char line[1024];
	int depth = 0;
	bool inDatabases = false;
	bool inSection = false;
	Entry cur;
	memset(&cur, 0, sizeof(cur));
	cur.mode = MysqlSsl_Preferred;

	while (fgets(line, sizeof(line), fp))
	{
		char *p = line;
		while (*p && isspace((unsigned char)*p))
			p++;
		if (*p == '\0' || *p == '#')
			continue;
		if (p[0] == '/' && p[1] == '/')
			continue;

		char *cmt = strstr(p, "//");
		if (cmt)
			*cmt = '\0';

		char buf[1024];
		strncpy(buf, p, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		TrimInPlace(buf);
		if (!buf[0])
			continue;

		if (buf[0] == '{')
		{
			depth++;
			continue;
		}
		if (buf[0] == '}')
		{
			if (inSection && depth == 2)
			{
				if (m_Count < (sizeof(m_Entries) / sizeof(m_Entries[0])) && cur.host[0])
					m_Entries[m_Count++] = cur;
				inSection = false;
				memset(&cur, 0, sizeof(cur));
				cur.mode = MysqlSsl_Preferred;
			}
			if (depth > 0)
				depth--;
			if (depth == 0)
				inDatabases = false;
			continue;
		}

		if (buf[0] == '"')
		{
			char name[256];
			strncpy(name, buf, sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';
			/* Only the first quoted token is the section name. */
			char *end = strchr(name + 1, '"');
			if (end)
				end[1] = '\0';
			Unquote(name);

			if (!inDatabases && strcasecmp(name, "Databases") == 0)
			{
				inDatabases = true;
				continue;
			}
			if (inDatabases && depth == 1 && strchr(buf + 1, '"') != strrchr(buf, '"'))
			{
				/* key/value line with two quoted strings — fall through */
			}
			else if (inDatabases && depth == 1)
			{
				/* Count quotes: section title has a single pair. */
				int quotes = 0;
				for (char *c = buf; *c; c++)
					if (*c == '"')
						quotes++;
				if (quotes == 2)
				{
					inSection = true;
					memset(&cur, 0, sizeof(cur));
					cur.mode = MysqlSsl_Preferred;
					continue;
				}
			}
		}

		if (inSection && depth >= 2)
		{
			char key[128] = {0};
			char value[512] = {0};
			char *q1 = strchr(buf, '"');
			if (!q1)
				continue;
			char *q2 = strchr(q1 + 1, '"');
			if (!q2)
				continue;
			size_t klen = (size_t)(q2 - q1 - 1);
			if (klen >= sizeof(key))
				klen = sizeof(key) - 1;
			memcpy(key, q1 + 1, klen);
			key[klen] = '\0';

			char *q3 = strchr(q2 + 1, '"');
			if (!q3)
				continue;
			char *q4 = strchr(q3 + 1, '"');
			if (!q4)
				continue;
			size_t vlen = (size_t)(q4 - q3 - 1);
			if (vlen >= sizeof(value))
				vlen = sizeof(value) - 1;
			memcpy(value, q3 + 1, vlen);
			value[vlen] = '\0';

			if (strcasecmp(key, "host") == 0)
				strncpy(cur.host, value, sizeof(cur.host) - 1);
			else if (strcasecmp(key, "user") == 0)
				strncpy(cur.user, value, sizeof(cur.user) - 1);
			else if (strcasecmp(key, "database") == 0)
				strncpy(cur.database, value, sizeof(cur.database) - 1);
			else if (strcasecmp(key, "port") == 0)
			{
				cur.port = (unsigned int)atoi(value);
				cur.hasPort = true;
			}
			else if (strcasecmp(key, "ssl_mode") == 0)
				cur.mode = ParseMode(value);
			else if (strcasecmp(key, "ssl_ca") == 0)
				strncpy(cur.ca, value, sizeof(cur.ca) - 1);
		}
	}

	fclose(fp);
}

bool MysqlSslConfig::Match(const SourceMod::DatabaseInfo *info, size_t index) const
{
	const Entry &e = m_Entries[index];
	if (!info->host || strcmp(e.host, info->host) != 0)
		return false;
	if (!info->user || strcmp(e.user, info->user) != 0)
		return false;
	if (!info->database || strcmp(e.database, info->database) != 0)
		return false;
	if (e.hasPort && e.port != info->port)
		return false;
	return true;
}

void MysqlSslConfig::Lookup(const SourceMod::DatabaseInfo *info, MysqlSslOptions *out)
{
	out->mode = MysqlSsl_Preferred;
	out->caPath[0] = '\0';
	Reload();

	for (size_t i = 0; i < m_Count; i++)
	{
		if (!Match(info, i))
			continue;
		out->mode = m_Entries[i].mode;
		if (m_Entries[i].ca[0])
		{
			if (m_Entries[i].ca[0] == '/')
				strncpy(out->caPath, m_Entries[i].ca, sizeof(out->caPath) - 1);
			else if (smutils)
				smutils->BuildPath(Path_SM, out->caPath, sizeof(out->caPath), "%s", m_Entries[i].ca);
			else
				strncpy(out->caPath, m_Entries[i].ca, sizeof(out->caPath) - 1);
		}
		return;
	}
}

void ApplyMysqlSslOptions(MYSQL *mysql, const MysqlSslOptions *opt)
{
	my_bool off = 0;
	my_bool on = 1;

	mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char *)&off);

	switch (opt->mode)
	{
	case MysqlSsl_Disabled:
		break;
	case MysqlSsl_Preferred:
	case MysqlSsl_Required:
		mysql_options(mysql, MYSQL_OPT_SSL_ENFORCE, (const char *)&on);
		break;
	case MysqlSsl_VerifyCA:
	case MysqlSsl_VerifyIdentity:
		mysql_options(mysql, MYSQL_OPT_SSL_ENFORCE, (const char *)&on);
		mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (const char *)&on);
		if (opt->caPath[0])
			mysql_options(mysql, MYSQL_OPT_SSL_CA, opt->caPath);
		break;
	}

#ifdef MARIADB_OPT_TLS_VERSION
	mysql_options(mysql, MARIADB_OPT_TLS_VERSION, (void *)"TLSv1.2,TLSv1.3");
#endif
}
