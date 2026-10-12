/**
 * Extra databases.cfg keys (ssl_mode / ssl_ca) are ignored by SourceMod core.
 * This helper re-parses configs/databases.cfg and matches by connection tuple.
 */

#ifndef _INCLUDE_SM_MYSQL_SSL_CONFIG_H_
#define _INCLUDE_SM_MYSQL_SSL_CONFIG_H_

#include <IDBDriver.h>
#include <sm_platform.h>
#include <mysql.h>
#include <stddef.h>

enum MysqlSslMode
{
	MysqlSsl_Disabled = 0,
	MysqlSsl_Preferred,
	MysqlSsl_Required,
	MysqlSsl_VerifyCA,
	MysqlSsl_VerifyIdentity
};

struct MysqlSslOptions
{
	MysqlSslMode mode;
	char caPath[PLATFORM_MAX_PATH];
};

class MysqlSslConfig
{
public:
	void Init();
	void Reload();
	void Lookup(const SourceMod::DatabaseInfo *info, MysqlSslOptions *out);

private:
	void ParseFile(const char *path);
	bool Match(const SourceMod::DatabaseInfo *info, size_t index) const;

	struct Entry
	{
		char host[256];
		char user[128];
		char database[128];
		unsigned int port;
		MysqlSslMode mode;
		char ca[PLATFORM_MAX_PATH];
		bool hasPort;
	};

	Entry m_Entries[64];
	size_t m_Count;
	bool m_Loaded;
	char m_Path[PLATFORM_MAX_PATH];
	long long m_Mtime;
};

extern MysqlSslConfig g_MysqlSslConfig;

void ApplyMysqlSslOptions(MYSQL *mysql, const MysqlSslOptions *opt);

#endif /* _INCLUDE_SM_MYSQL_SSL_CONFIG_H_ */
