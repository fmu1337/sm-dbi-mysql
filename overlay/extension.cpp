/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * sm-dbi-mysql v2: DBI driver entry point.
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

#include "extension.h"
#include "mysql/MyDriver.h"
#include <stdio.h>
#include <stdlib.h>
#include <openssl/crypto.h>

/* Set by the build (scripts/prepare.sh / Makefile): the SourceMod SDK label, e.g. sm17. */
#ifndef SM_DBI_LABEL
#define SM_DBI_LABEL "unlabeled"
#endif

DBI_MySQL g_MySqlDBI;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_MySqlDBI);

bool DBI_MySQL::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	if (mysql_library_init(0, NULL, NULL) != 0) {
		smutils->Format(error, maxlength, "Could not initialize MySQL client library");
		return false;
	}

	dbi->AddDriver(&g_MyDriver);

	return true;
}

void DBI_MySQL::SDK_OnUnload()
{
	dbi->RemoveDriver(&g_MyDriver);

	mysql_library_end();
}

/* "2.0.0-sm17 (MariaDB C/C 3.4.9, OpenSSL 3.0.22 ...)": what `sm exts list` shows. */
const char *DBI_MySQL::GetExtensionVerString()
{
	static char buffer[256];
	if (!buffer[0])
	{
		snprintf(buffer, sizeof(buffer), "%s-%s (MariaDB C/C %s, %s)",
			SMEXT_CONF_VERSION, SM_DBI_LABEL,
			mysql_get_client_info(), OpenSSL_version(OPENSSL_VERSION));
	}
	return buffer;
}

const char *DBI_MySQL::GetExtensionDateString()
{
	return SMEXT_CONF_DATESTRING;
}
