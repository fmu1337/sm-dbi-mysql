/**
 * SourceMod MySQL Extension — MariaDB Connector/C compatible headers.
 */

#ifndef _INCLUDE_SM_MYSQL_DRIVER_H_
#define _INCLUDE_SM_MYSQL_DRIVER_H_

#define SOURCEMOD_SQL_DRIVER_CODE
#include <IDBDriver.h>
#include <sm_platform.h>
#if defined PLATFORM_WINDOWS
#include <WinSock2.h>
#endif

#include <mysql.h>

#if defined(min)
# undef min
#endif
#if defined(max)
# undef max
#endif

#include <sh_string.h>
#include <sh_list.h>
#include <mutex>

using namespace SourceMod;
using namespace SourceHook;

#define M_CLIENT_MULTI_RESULTS    ((1) << 17)

class MyDatabase;

class MyDriver : public IDBDriver
{
public:
	MyDriver();
public:
	IDatabase *Connect(const DatabaseInfo *info, bool persistent, char *error, size_t maxlength);
	const char *GetIdentifier();
	const char *GetProductName();
	Handle_t GetHandle();
	IdentityToken_t *GetIdentity();
	bool IsThreadSafe();
	bool InitializeThreadSafety();
	void ShutdownThreadSafety();
public:
	void Shutdown();
	void RemoveFromList(MyDatabase *pdb, bool persistent);
private:
	std::mutex m_Lock;
	Handle_t m_MyHandle;
	List<MyDatabase *> m_TempDbs;
	List<MyDatabase *> m_PermDbs;
};

extern MyDriver g_MyDriver;

size_t strncopy(char *dest, const char *src, size_t count);

#endif /* _INCLUDE_SM_MYSQL_DRIVER_H_ */
