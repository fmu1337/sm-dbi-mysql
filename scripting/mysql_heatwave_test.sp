/**
 * sm-dbi-mysql v2: driver test plugin.
 *
 * Commands
 *   sm_mysql_hw_test          full driver test on the configs below (prints PASS/FAIL).
 *   sm_mysql_hw_ssl [prefix]  ssl_mode matrix: connects with <prefix>_disabled,
 *                             <prefix>_preferred, <prefix>_required, <prefix>_verify_ca,
 *                             <prefix>_verify_identity and prints the outcome and the
 *                             TLS cipher of each (default prefix: mysql_ssl).
 *
 * databases.cfg sections used (see README.md for examples):
 *   mysql_test_a, mysql_test_b   same server, user, password, database and port, same
 *                                ssl settings: SQL_Connect() returns one shared
 *                                persistent connection for both.
 *   mysql_test_killer            same server and user, database "information_schema":
 *                                a separate connection, used to KILL the shared one.
 *
 * Compile: spcomp mysql_heatwave_test.sp (any SourceMod 1.5 - 1.13 compiler).
 * Uses only natives that exist in SourceMod 1.5.
 */

#include <sourcemod>

#define TEST_TAG "[MYSQL-HW]"
#define THREADED_QUERIES 40
#define SYNC_QUERIES 40

public Plugin myinfo =
{
	name = "sm-dbi-mysql driver test",
	author = "fmu1337",
	description = "Threaded/sync queries on a shared handle, killed connection, utf8mb4, prepared statements, transactions, ssl_mode",
	version = "2.0.0",
	url = "https://github.com/fmu1337/sm-dbi-mysql"
};

int g_IdA;
int g_ThreadedDone;
int g_ThreadedErrors;
int g_ThreadedSeen;
Database g_TestDb;
int g_Passed;
int g_Failed;

public void OnPluginStart()
{
	RegAdminCmd("sm_mysql_hw_test", Cmd_Test, ADMFLAG_RCON, "Run the sm-dbi-mysql driver test");
	RegAdminCmd("sm_mysql_hw_ssl", Cmd_Ssl, ADMFLAG_RCON, "Connect with each ssl_mode and print the outcome");
}

static void Check(bool ok, const char[] name, const char[] fmt = "", any ...)
{
	char detail[192];
	VFormat(detail, sizeof(detail), fmt, 4);
	if (ok)
	{
		g_Passed++;
		PrintToServer("%s PASS %s %s", TEST_TAG, name, detail);
	}
	else
	{
		g_Failed++;
		PrintToServer("%s FAIL %s %s", TEST_TAG, name, detail);
	}
}

/* ------------------------------------------------------------------------ */
/* Full test                                                                */
/* ------------------------------------------------------------------------ */

public Action Cmd_Test(int client, int args)
{
	g_Passed = 0;
	g_Failed = 0;
	char error[256];

	Database a = SQL_Connect("mysql_test_a", true, error, sizeof(error));
	if (a == null)
	{
		PrintToServer("%s cannot connect mysql_test_a: %s", TEST_TAG, error);
		return Plugin_Handled;
	}
	Database b = SQL_Connect("mysql_test_b", true, error, sizeof(error));
	if (b == null)
	{
		PrintToServer("%s cannot connect mysql_test_b: %s", TEST_TAG, error);
		delete a;
		return Plugin_Handled;
	}
	g_TestDb = a;

	/* Shared persistent connection: both configs report the same CONNECTION_ID(). */
	int idA = ConnectionId(a);
	int idB = ConnectionId(b);
	Check(idA > 0 && idA == idB, "shared_handle", "(ids a=%d b=%d)", idA, idB);

	/* Synchronous queries on the main thread, interleaved with threaded queries on the same handle. */
	g_ThreadedDone = 0;
	g_ThreadedErrors = 0;
	g_ThreadedSeen = 0;
	for (int i = 0; i < THREADED_QUERIES; i++)
	{
		SQL_TQuery(a, OnThreadedQuery, "SELECT SLEEP(0.01), 1", i);
	}

	int syncErrors = 0;
	for (int i = 0; i < SYNC_QUERIES; i++)
	{
		DBResultSet rs = SQL_Query(a, "SELECT 1");
		if (rs == null)
		{
			syncErrors++;
		}
		else
		{
			delete rs;
		}
	}
	Check(syncErrors == 0, "sync_queries_during_threaded", "(errors=%d)", syncErrors);

	/* The killed-connection case runs in Timer_WaitThreaded, after every threaded query has
	 * finished, so no query is in flight when the connection is killed. */
	g_IdA = idA;

	/* utf8mb4 round trip through a temporary table. */
	SQL_FastQuery(a, "CREATE TEMPORARY TABLE IF NOT EXISTS sm_hw_utf8 (id INT, s VARCHAR(64) CHARACTER SET utf8mb4) CHARACTER SET utf8mb4");
	char utf[] = "emoji 😀 ñ 中";
	char read[128];
	char err[256];
	DBStatement st = SQL_PrepareQuery(a, "INSERT INTO sm_hw_utf8 (id, s) VALUES (?, ?)", err, sizeof(err));
	if (st != null)
	{
		SQL_BindParamInt(st, 0, 1);
		SQL_BindParamString(st, 1, utf, false);
		Check(SQL_Execute(st), "prepared_insert_utf8mb4");
		delete st;
	}
	else
	{
		Check(false, "prepared_insert_utf8mb4", err);
	}
	DBResultSet rs = SQL_Query(a, "SELECT s FROM sm_hw_utf8 WHERE id = 1");
	if (rs != null && rs.FetchRow())
	{
		rs.FetchString(0, read, sizeof(read));
		Check(StrEqual(read, utf), "utf8mb4_round_trip", read);
	}
	else
	{
		Check(false, "utf8mb4_round_trip", "(no row)");
	}
	delete rs;

	/* Prepared SELECT with a bound string and an int, fetched back. */
	st = SQL_PrepareQuery(a, "SELECT id, s FROM sm_hw_utf8 WHERE id = ? AND s = ?", err, sizeof(err));
	if (st != null)
	{
		SQL_BindParamInt(st, 0, 1);
		SQL_BindParamString(st, 1, utf, false);
		bool ok = SQL_Execute(st) && SQL_FetchRow(st);
		Check(ok && SQL_FetchInt(st, 0) == 1, "prepared_select");
		delete st;
	}
	else
	{
		Check(false, "prepared_select", err);
	}

	/* Transactions: rollback leaves no row; commit keeps it. */
	SQL_FastQuery(a, "CREATE TEMPORARY TABLE IF NOT EXISTS sm_hw_txn (id INT)");
	SQL_FastQuery(a, "START TRANSACTION");
	SQL_FastQuery(a, "INSERT INTO sm_hw_txn VALUES (1)");
	SQL_FastQuery(a, "ROLLBACK");
	rs = SQL_Query(a, "SELECT COUNT(*) FROM sm_hw_txn");
	int rolledBack = (rs != null && rs.FetchRow()) ? rs.FetchInt(0) : -1;
	delete rs;
	Check(rolledBack == 0, "transaction_rollback", "(rows=%d)", rolledBack);

	SQL_FastQuery(a, "START TRANSACTION");
	SQL_FastQuery(a, "INSERT INTO sm_hw_txn VALUES (2)");
	SQL_FastQuery(a, "COMMIT");
	rs = SQL_Query(a, "SELECT COUNT(*) FROM sm_hw_txn");
	int committed = (rs != null && rs.FetchRow()) ? rs.FetchInt(0) : -1;
	delete rs;
	Check(committed == 1, "transaction_commit", "(rows=%d)", committed);

	/* Threaded results: wait for the callbacks (up to 30 s) before the summary. */
	CreateTimer(0.2, Timer_WaitThreaded, a, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
	return Plugin_Handled;
}

/* Another session kills the shared connection; the next query on the same handle must not
 * crash and must either reconnect or fail with a message. Runs with no query in flight. */
static void KillCase(Database a, int idA)
{
	char error[256];
	Database killer = SQL_Connect("mysql_test_killer", true, error, sizeof(error));
	if (killer == null)
	{
		PrintToServer("%s note: mysql_test_killer unavailable (%s); kill case skipped", TEST_TAG, error);
		return;
	}
	char query[64];
	Format(query, sizeof(query), "KILL CONNECTION %d", idA);
	SQL_FastQuery(killer, query);
	delete killer;

	DBResultSet afterKill = SQL_Query(a, "SELECT 1");
	bool afterKillOk = (afterKill != null);
	delete afterKill;
	int idAfter = ConnectionId(a);
	Check(afterKillOk || idAfter > 0, "killed_connection_no_crash", "(ok=%d new_id=%d)", afterKillOk, idAfter);
}

public Action Timer_WaitThreaded(Handle timer, any data)
{
	static int ticks;
	ticks++;
	if (g_ThreadedDone < THREADED_QUERIES && ticks < 150)
	{
		return Plugin_Continue;
	}

	ticks = 0;
	/* Every callback has run (or the wait timed out): no query is in flight now. */
	Check(g_ThreadedDone == THREADED_QUERIES && g_ThreadedErrors == 0,
		"threaded_queries_on_shared_handle",
		"(done=%d errors=%d)", g_ThreadedDone, g_ThreadedErrors);

	if (g_ThreadedDone == THREADED_QUERIES)
	{
		KillCase(g_TestDb, g_IdA);
	}

	delete g_TestDb;
	g_TestDb = null;
	PrintToServer("%s SUMMARY passed=%d failed=%d", TEST_TAG, g_Passed, g_Failed);
	return Plugin_Stop;
}

/*
 * SQL_TQuery callback. SourceMod 1.7 and older declare it with Handle arguments,
 * 1.8+ with Database/DBResultSet. A compiler that predates the typed form needs
 * -DSM_LEGACY_TQUERY (the CI builds the 1.7 variant with it).
 */
#if defined SM_LEGACY_TQUERY
public void OnThreadedQuery(Handle owner, Handle hndl, const char[] error, any data)
{
	ThreadedResult(hndl != null, error);
}
#else
public void OnThreadedQuery(Database db, DBResultSet results, const char[] error, any data)
{
	ThreadedResult(results != null, error);
}
#endif

static void ThreadedResult(bool ok, const char[] error)
{
	g_ThreadedDone++;
	if (!ok)
	{
		g_ThreadedErrors++;
		if (g_ThreadedErrors == 1)
		{
			PrintToServer("%s threaded error: %s", TEST_TAG, error);
		}
	}
	else
	{
		g_ThreadedSeen++;
	}
}

static int ConnectionId(Database db)
{
	DBResultSet rs = SQL_Query(db, "SELECT CONNECTION_ID()");
	int id = -1;
	if (rs != null && rs.FetchRow())
	{
		id = rs.FetchInt(0);
	}
	delete rs;
	return id;
}

/* ------------------------------------------------------------------------ */
/* ssl_mode matrix                                                          */
/* ------------------------------------------------------------------------ */

static void SslProbe(const char[] prefix, const char[] mode)
{
	char name[96], error[256];
	Format(name, sizeof(name), "%s_%s", prefix, mode);

	Database db = SQL_Connect(name, true, error, sizeof(error));
	if (db == null)
	{
		PrintToServer("%s ssl_mode=%s config=%s result=FAIL error=\"%s\"", TEST_TAG, mode, name, error);
		return;
	}

	char cipher[128] = "";
	DBResultSet rs = SQL_Query(db, "SHOW STATUS LIKE 'Ssl_cipher'");
	if (rs != null && rs.FetchRow())
	{
		rs.FetchString(1, cipher, sizeof(cipher));
	}
	delete rs;
	PrintToServer("%s ssl_mode=%s config=%s result=OK cipher=\"%s\"", TEST_TAG, mode, name, cipher);
	delete db;
}

public Action Cmd_Ssl(int client, int args)
{
	char prefix[64] = "mysql_ssl";
	if (args >= 1)
	{
		GetCmdArg(1, prefix, sizeof(prefix));
	}

	SslProbe(prefix, "disabled");
	SslProbe(prefix, "preferred");
	SslProbe(prefix, "required");
	SslProbe(prefix, "verify_ca");
	SslProbe(prefix, "verify_identity");
	return Plugin_Handled;
}
