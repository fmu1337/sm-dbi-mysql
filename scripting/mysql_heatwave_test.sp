#include <sourcemod>

public Plugin:myinfo =
{
	name = "MySQL HeatWave DBI test",
	author = "fmu1337",
	description = "Acceptance tests for modern dbi.mysql (caching_sha2 / TLS)",
	version = "1.0.0",
	url = "https://github.com/fmu1337/sm-dbi-mysql"
};

new Handle:g_DB = INVALID_HANDLE;

public OnPluginStart()
{
	RegServerCmd("sm_mysql_hw_test", Cmd_Test, "Run HeatWave/MySQL DBI acceptance tests");
}

public Action:Cmd_Test(args)
{
	decl String:error[256];
	g_DB = SQL_Connect("default", true, error, sizeof(error));
	if (g_DB == INVALID_HANDLE)
	{
		PrintToServer("[mysql_hw_test] Connect failed: %s", error);
		return Plugin_Handled;
	}
	PrintToServer("[mysql_hw_test] Connected OK");

	if (!SQL_FastQuery(g_DB, "CREATE TABLE IF NOT EXISTS sm_hw_test (id INT AUTO_INCREMENT PRIMARY KEY, nick VARCHAR(128) NOT NULL, created TIMESTAMP DEFAULT CURRENT_TIMESTAMP) CHARACTER SET utf8mb4"))
	{
		SQL_GetError(g_DB, error, sizeof(error));
		PrintToServer("[mysql_hw_test] CREATE failed: %s", error);
		CloseHandle(g_DB);
		g_DB = INVALID_HANDLE;
		return Plugin_Handled;
	}

	decl String:nick[128], String:esc[256], String:query[512];
	strcopy(nick, sizeof(nick), "Test nick quotes ' \" and utf8");
	SQL_EscapeString(g_DB, nick, esc, sizeof(esc));
	Format(query, sizeof(query), "INSERT INTO sm_hw_test (nick) VALUES ('%s')", esc);

	if (!SQL_FastQuery(g_DB, query))
	{
		SQL_GetError(g_DB, error, sizeof(error));
		PrintToServer("[mysql_hw_test] INSERT failed: %s", error);
		CloseHandle(g_DB);
		g_DB = INVALID_HANDLE;
		return Plugin_Handled;
	}
	PrintToServer("[mysql_hw_test] insert_id=%d affected=%d", SQL_GetInsertId(g_DB), SQL_GetAffectedRows(g_DB));

	new Handle:rs = SQL_Query(g_DB, "SELECT id, nick FROM sm_hw_test ORDER BY id DESC LIMIT 1");
	if (rs == INVALID_HANDLE)
	{
		SQL_GetError(g_DB, error, sizeof(error));
		PrintToServer("[mysql_hw_test] SELECT failed: %s", error);
	}
	else
	{
		if (SQL_FetchRow(rs))
		{
			decl String:got[128];
			SQL_FetchString(rs, 1, got, sizeof(got));
			PrintToServer("[mysql_hw_test] row id=%d nick=%s", SQL_FetchInt(rs, 0), got);
		}
		CloseHandle(rs);
	}

	SQL_LockDatabase(g_DB);
	SQL_FastQuery(g_DB, "START TRANSACTION");
	SQL_FastQuery(g_DB, "UPDATE sm_hw_test SET nick=CONCAT(nick,'_tx') ORDER BY id DESC LIMIT 1");
	SQL_FastQuery(g_DB, "COMMIT");
	SQL_UnlockDatabase(g_DB);
	PrintToServer("[mysql_hw_test] transaction OK");

	new i;
	for (i = 0; i < 1000; i++)
	{
		if (!SQL_FastQuery(g_DB, "SELECT 1"))
		{
			SQL_GetError(g_DB, error, sizeof(error));
			PrintToServer("[mysql_hw_test] burst fail at %d: %s", i, error);
			break;
		}
	}
	if (i == 1000)
		PrintToServer("[mysql_hw_test] 1000 queries OK");

	CloseHandle(g_DB);
	g_DB = INVALID_HANDLE;
	PrintToServer("[mysql_hw_test] done");
	return Plugin_Handled;
}
