/*
 * sm-dbi-mysql v2: authentication check for the MariaDB Connector/C build.
 *
 * Linked statically against the same libmariadbclient.a and OpenSSL that the
 * extension uses (scripts/auth-test.sh), so a missing auth plugin shows up here
 * instead of on a game server ("Plugin caching_sha2_password could not be loaded").
 *
 * Creates two caching_sha2_password users on a MySQL 8.4 server. Each one logs in
 * for the first time, which is a full authentication (no cached hash):
 *   sm_tls    over TLS (verification off, as the driver's required mode does)
 *   sm_plain  over plaintext, which needs the server's RSA public key
 * Then each logs in again (fast authentication from the server cache).
 *
 * usage: auth_test <host> <port> <root_password>
 */
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static MYSQL *connect_as(const char *host, unsigned int port, const char *user, const char *pass, int tls)
{
	MYSQL *m = mysql_init(NULL);
	my_bool no = 0, yes = 1;
	if (!m) {
		return NULL;
	}
	mysql_options(m, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &no);
	if (tls) {
		mysql_options(m, MYSQL_OPT_SSL_ENFORCE, &yes);
	}
	if (!mysql_real_connect(m, host, user, pass, NULL, port, NULL, 0)) {
		fprintf(stderr, "%s (%s): [%u] %s\n", user, tls ? "tls" : "plain",
			mysql_errno(m), mysql_error(m));
		mysql_close(m);
		return NULL;
	}
	return m;
}

static void check(const char *host, unsigned int port, const char *user, const char *pass, int tls)
{
	for (int round = 1; round <= 2; round++) {
		MYSQL *m = connect_as(host, port, user, pass, tls);
		const char *cipher;
		if (!m) {
			fails++;
			continue;
		}
		cipher = mysql_get_ssl_cipher(m);
		printf("ok   %-9s %-5s login %d  tls=%s\n", user, tls ? "tls" : "plain", round,
			cipher && cipher[0] ? cipher : "off");
		if (tls && (!cipher || !cipher[0])) {
			fprintf(stderr, "FAIL %s: TLS was requested but the session is plain\n", user);
			fails++;
		}
		if (!tls && cipher && cipher[0]) {
			fprintf(stderr, "FAIL %s: plaintext was requested but the session is encrypted\n", user);
			fails++;
		}
		mysql_close(m);
	}
}

static int run(MYSQL *root, const char *sql)
{
	if (mysql_query(root, sql)) {
		fprintf(stderr, "setup: %s\n  %s\n", sql, mysql_error(root));
		return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <host> <port> <root_password>\n", argv[0]);
		return 2;
	}
	const char *host = argv[1];
	unsigned int port = (unsigned int)atoi(argv[2]);
	const char *root_pw = argv[3];

	printf("client %s\n", mysql_get_client_info());

	MYSQL *root = connect_as(host, port, "root", root_pw, 0);
	if (!root) {
		fprintf(stderr, "cannot log in as root (is the server up?)\n");
		return 1;
	}
	run(root, "DROP USER IF EXISTS 'sm_tls'@'%'");
	run(root, "DROP USER IF EXISTS 'sm_plain'@'%'");
	run(root, "CREATE USER 'sm_tls'@'%' IDENTIFIED WITH caching_sha2_password BY 'tls-pw-123'");
	run(root, "CREATE USER 'sm_plain'@'%' IDENTIFIED WITH caching_sha2_password BY 'plain-pw-123'");
	run(root, "GRANT SELECT ON *.* TO 'sm_tls'@'%', 'sm_plain'@'%'");
	if (fails) {
		mysql_close(root);
		return 1;
	}
	mysql_close(root);

	check(host, port, "sm_tls", "tls-pw-123", 1);
	check(host, port, "sm_plain", "plain-pw-123", 0);

	if (fails) {
		fprintf(stderr, "auth-test: %d failure(s)\n", fails);
		return 1;
	}
	printf("auth-test: all logins OK\n");
	return 0;
}
