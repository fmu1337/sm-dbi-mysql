/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * sm-dbi-mysql v2: stand-in for amtl's am-refcounting-threadsafe.h, for the
 * sm15 build only (SourceMod 1.5 ships no amtl). Same contract: a thread-safe
 * intrusive refcount; the object deletes itself when the count reaches zero.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 */

#ifndef _INCLUDE_SM_MYSQL_COMPAT_REFCOUNTING_THREADSAFE_H_
#define _INCLUDE_SM_MYSQL_COMPAT_REFCOUNTING_THREADSAFE_H_

#include <atomic>

namespace ke {

template <typename T>
class RefcountedThreadsafe
{
public:
	RefcountedThreadsafe() : refcount_(0) {}
	virtual ~RefcountedThreadsafe() {}

	void AddRef() { refcount_.fetch_add(1, std::memory_order_relaxed); }
	// Returns true when this call dropped the last reference and the object was deleted.
	bool Release() {
		if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			delete static_cast<T *>(this);
			return true;
		}
		return false;
	}

private:
	std::atomic<unsigned int> refcount_;
};

} // namespace ke

#endif // _INCLUDE_SM_MYSQL_COMPAT_REFCOUNTING_THREADSAFE_H_
