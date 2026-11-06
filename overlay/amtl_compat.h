/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * sm-dbi-mysql v2: compatibility for SourceMod 1.5 - 1.7 SDKs (amtl).
 * =============================================================================
 *
 * Forced into every source file of the sm15, sm16 and sm17 builds (see the
 * Makefile). The upstream extensions/mysql source at the pin uses two amtl
 * helpers that those SDKs do not have: ke::RefPtr (held references to the
 * database in the result and statement objects) and ke::SafeSprintf. Both are
 * defined here with the same semantics. Newer SDKs have them; this file is not
 * included for sm18 and later.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 */

#ifndef _INCLUDE_SM_MYSQL_AMTL_COMPAT_H_
#define _INCLUDE_SM_MYSQL_AMTL_COMPAT_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

namespace ke {

/* Strong reference to a refcounted object (AddRef/Release). */
template <typename T>
class RefPtr
{
public:
	RefPtr() : ptr_(NULL) {}
	RefPtr(T *ptr) : ptr_(NULL) { set(ptr); }
	RefPtr(const RefPtr &other) : ptr_(NULL) { set(other.ptr_); }
	~RefPtr() { set(NULL); }
	RefPtr &operator=(const RefPtr &other) { set(other.ptr_); return *this; }
	RefPtr &operator=(T *ptr) { set(ptr); return *this; }
	T *operator->() const { return ptr_; }
	T *get() const { return ptr_; }
	operator T *() const { return ptr_; }

private:
	void set(T *ptr) {
		if (ptr)
			ptr->AddRef();
		T *old = ptr_;
		ptr_ = ptr;
		if (old)
			old->Release();
	}

	T *ptr_;
};

/* Like snprintf, always NUL-terminated; returns the length that was written. */
inline int SafeSprintf(char *buffer, size_t maxlen, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int written = vsnprintf(buffer, maxlen, fmt, ap);
	va_end(ap);
	if (maxlen > 0 && written >= (int)maxlen)
		written = (int)maxlen - 1;
	return written;
}

} // namespace ke

#endif // _INCLUDE_SM_MYSQL_AMTL_COMPAT_H_
