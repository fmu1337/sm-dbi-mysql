/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * sm-dbi-mysql v2: extension configuration (overrides the SDK's extensions/mysql
 * smsdk_config.h; see vendor/sourcemod-mysql/smsdk_config.h for the upstream one).
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

/* Basic information exposed publicly */
#define SMEXT_CONF_NAME			"MySQL-DBI"
#define SMEXT_CONF_DESCRIPTION	"MySQL driver for DBI (MariaDB C/C, caching_sha2, TLS, ssl_mode)"
#define SMEXT_CONF_VERSION		"2.0.0"
#define SMEXT_CONF_AUTHOR		"AlliedModders LLC, fmu1337"
#define SMEXT_CONF_URL			"https://github.com/fmu1337/sm-dbi-mysql"
#define SMEXT_CONF_LOGTAG		"MYSQL"
#define SMEXT_CONF_LICENSE		"GPL-3.0-only"
#define SMEXT_CONF_DATESTRING	__DATE__

/** Exposes plugin's main interface. */
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

/* Metamod is not used by the DBI driver. */
//#define SMEXT_CONF_METAMOD

/* Interfaces used by the driver. */
#define SMEXT_ENABLE_DBMANAGER
#define SMEXT_ENABLE_TEXTPARSERS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
