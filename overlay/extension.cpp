/**
 * SourceMod MySQL Extension
 */

#include "extension.h"
#include "mysql/MyDriver.h"
#include "mysql/MysqlSslConfig.h"
#include <stdlib.h>

DBI_MySQL g_MySqlDBI;

SMEXT_LINK(&g_MySqlDBI);

bool DBI_MySQL::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	if (mysql_library_init(0, NULL, NULL) != 0) {
		smutils->Format(error, maxlength, "Could not initialize MySQL client library");
		return false;
	}

	g_MysqlSslConfig.Init();
	dbi->AddDriver(&g_MyDriver);
	return true;
}

void DBI_MySQL::SDK_OnUnload()
{
	dbi->RemoveDriver(&g_MyDriver);
	mysql_library_end();
}

const char *DBI_MySQL::GetExtensionVerString()
{
	return SMEXT_CONF_VERSION;
}

const char *DBI_MySQL::GetExtensionDateString()
{
	return SMEXT_CONF_DATESTRING;
}
