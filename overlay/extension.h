/**
 * SourceMod MySQL Extension
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"

class DBI_MySQL : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	virtual void SDK_OnUnload();
	const char *GetExtensionVerString();
	const char *GetExtensionDateString();
};

#endif /* _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_ */
