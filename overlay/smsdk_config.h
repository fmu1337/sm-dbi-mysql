/**
 * SourceMod MySQL Extension (HeatWave / MySQL 8+ backport)
 * Based on AlliedModders SourceMod MySQL-DBI.
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"MySQL-DBI"
#define SMEXT_CONF_DESCRIPTION	"MySQL driver for DBI (MariaDB Connector/C, caching_sha2, TLS)"
#define SMEXT_CONF_VERSION		"1.0.0"
#define SMEXT_CONF_AUTHOR		"AlliedModders LLC / fmu1337"
#define SMEXT_CONF_URL			"https://github.com/fmu1337/sm-dbi-mysql"
#define SMEXT_CONF_LOGTAG		"MYSQL"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

/* Metamod not required for DBI driver */
//#define SMEXT_CONF_METAMOD

#define SMEXT_ENABLE_DBMANAGER

#endif /* _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_ */
