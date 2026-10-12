/**
 * SourceMod MySQL Extension — Connect() with TLS / utf8mb4 / MariaDB Connector/C.
 */

#include "MyDriver.h"
#include "MyDatabase.h"
#include "MysqlSslConfig.h"
#include "smsdk_ext.h"
#include <stdio.h>
#include <string.h>

MyDriver g_MyDriver;

MyDriver::MyDriver()
{
	m_MyHandle = BAD_HANDLE;
}

void CloseDBList(List<MyDatabase *> &l)
{
	List<MyDatabase *>::iterator iter;
	for (iter = l.begin(); iter != l.end(); iter++)
	{
		MyDatabase *db = (*iter);
		while (!db->Close())
		{
		}
	}
	l.clear();
}

void MyDriver::Shutdown()
{
	CloseDBList(m_PermDbs);

	if (m_MyHandle != BAD_HANDLE)
	{
		dbi->ReleaseHandle(m_MyHandle, DBHandle_Driver, myself->GetIdentity());
		m_MyHandle = BAD_HANDLE;
	}
}

const char *MyDriver::GetIdentifier()
{
	return "mysql";
}

Handle_t MyDriver::GetHandle()
{
	if (m_MyHandle == BAD_HANDLE)
	{
		m_MyHandle = dbi->CreateHandle(DBHandle_Driver, this, myself->GetIdentity());
	}
	return m_MyHandle;
}

IdentityToken_t *MyDriver::GetIdentity()
{
	return myself->GetIdentity();
}

const char *MyDriver::GetProductName()
{
	return "MySQL";
}

MYSQL *Connect(const DatabaseInfo *info, char *error, size_t maxlength)
{
	MYSQL *mysql = mysql_init(NULL);
	const char *host = NULL, *socket = NULL;

	unsigned int timeout = 60;
	if (info->maxTimeout > 0)
	{
		timeout = info->maxTimeout;
	}

	mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
	mysql_options(mysql, MYSQL_OPT_READ_TIMEOUT, (const char *)&timeout);
	mysql_options(mysql, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&timeout);

	MysqlSslOptions ssl;
	g_MysqlSslConfig.Lookup(info, &ssl);
	ApplyMysqlSslOptions(mysql, &ssl);

	/* MYSQL_OPT_RECONNECT still present in MariaDB Connector/C. */
	my_bool my_true = true;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, (const char *)&my_true);

	/* Default charset; plugins may still SET NAMES afterwards. */
	mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if (info->host[0] == '/')
	{
		host = "localhost";
		socket = info->host;
	}
	else
	{
		host = info->host;
		socket = NULL;
	}

	if (!mysql_real_connect(mysql,
		host,
		info->user,
		info->pass,
		info->database,
		info->port,
		socket,
		M_CLIENT_MULTI_RESULTS))
	{
		snprintf(error, maxlength, "[%d]: %s", mysql_errno(mysql), mysql_error(mysql));
		mysql_close(mysql);
		return NULL;
	}

	if (ssl.mode == MysqlSsl_Required
		|| ssl.mode == MysqlSsl_VerifyCA
		|| ssl.mode == MysqlSsl_VerifyIdentity)
	{
		if (mysql_get_ssl_cipher(mysql) == NULL)
		{
			snprintf(error, maxlength, "[0]: TLS required but connection is not encrypted");
			mysql_close(mysql);
			return NULL;
		}
	}

	/* Ensure utf8mb4 even if server ignored MYSQL_SET_CHARSET_NAME. */
	if (mysql_set_character_set(mysql, "utf8mb4") != 0)
	{
		/* Non-fatal: keep connection; plugin SET NAMES may still work. */
	}

	return mysql;
}

bool CompareField(const char *str1, const char *str2)
{
	if ((str1 == NULL && str2 != NULL) || (str1 != NULL && str2 == NULL))
		return false;
	if (str1 == NULL && str2 == NULL)
		return true;
	return (strcmp(str1, str2) == 0);
}

IDatabase *MyDriver::Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength)
{
	std::lock_guard<std::mutex> lock(m_Lock);

	if (persistent)
	{
		List<MyDatabase *>::iterator iter;
		for (iter = m_PermDbs.begin(); iter != m_PermDbs.end(); iter++)
		{
			MyDatabase *db = (*iter);
			const DatabaseInfo &other = db->GetInfo();
			if (CompareField(info->host, other.host)
				&& CompareField(info->user, other.user)
				&& CompareField(info->pass, other.pass)
				&& CompareField(info->database, other.database)
				&& (info->port == other.port))
			{
				db->IncReferenceCount();
				return db;
			}
		}
	}

	MYSQL *mysql = ::Connect(info, error, maxlength);
	if (!mysql)
		return NULL;

	MyDatabase *db = new MyDatabase(mysql, info, persistent);
	if (persistent)
		m_PermDbs.push_back(db);
	return db;
}

void MyDriver::RemoveFromList(MyDatabase *pdb, bool persistent)
{
	std::lock_guard<std::mutex> lock(m_Lock);
	if (persistent)
		m_PermDbs.remove(pdb);
}

bool MyDriver::IsThreadSafe()
{
	return (mysql_thread_safe() != 0);
}

bool MyDriver::InitializeThreadSafety()
{
	return (mysql_thread_init() == 0);
}

void MyDriver::ShutdownThreadSafety()
{
	mysql_thread_end();
}

size_t strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
		return 0;
	char *start = dest;
	while ((*src) && (--count))
		*dest++ = *src++;
	*dest = '\0';
	return (size_t)(dest - start);
}
