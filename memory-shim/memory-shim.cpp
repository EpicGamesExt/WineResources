#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define PAGE_SIZE 4096

#include <algorithm>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

typedef void* (*MallocFnPtr)(size_t);
typedef void* (*MmapFnPtr)(void*, size_t, int, int, int, off_t);
typedef int (*MprotectFnPtr)(void*, size_t, int);

#define PrintError(format, args...) \
{ \
	fprintf(stderr, "[memory-shim]: Error: "); \
	fprintf(stderr, format, ## args); \
	fflush(stderr); \
}

template <typename T>
T GetRealFunction(const char* name)
{
	T ptr = (T)(dlsym(RTLD_NEXT, name));
	if (ptr == nullptr)
	{
		PrintError(
			"failed to retrieve function pointer for %s\n",
			name
		);
		exit(202);
	}
	
	return ptr;
}

bool ShimEnabled() {
	static int shim_enabled = -1;

	if (shim_enabled == -1)
	{
		const char *str = getenv( "ENABLE_MEMORY_SHIM" );

		shim_enabled = str && atoi(str) == 1;
	}

	return shim_enabled;
}

uint64_t GetMemlockLimit()
{
	struct rlimit limitValues = {};
	if (getrlimit(RLIMIT_MEMLOCK, &limitValues) == 0) {
		return limitValues.rlim_cur;
	}
	else
	{
		PrintError(
			"Failed to determine soft limit for RLIMIT_MEMLOCK: %s: %s\n",
			strerrorname_np(errno),
			strerror(errno)
		);
		return 0;
	}
}

int LockMemory(const void* addr, size_t len)
{
	int result = mlock(addr, len);
	if (result != 0)
	{
		PrintError(
			"mlock() failed: %s: %s\n",
			strerrorname_np(errno),
			strerror(errno)
		);
	}
	
	return result;
}

int UnlockMemory(const void* addr, size_t len)
{
	int result = munlock(addr, len);
	if (result != 0)
	{
		PrintError(
			"munlock() failed: %s: %s\n",
			strerrorname_np(errno),
			strerror(errno)
		);
	}
	
	return result;
}

bool TouchMemoryLock(void* addr, size_t len)
{
	#define CHECK_SUCCESS(expr) succeeded = succeeded && (expr == 0)
	static uint64_t limit = GetMemlockLimit();
	bool succeeded = true;
	
	if (limit == 0 || len <= limit)
	{
		CHECK_SUCCESS(LockMemory(addr, len));
		CHECK_SUCCESS(UnlockMemory(addr, len));
	}
	else
	{
		for (uint64_t current = 0; current < len; current += limit)
		{
			void* chunk = ((uint8_t*)addr) + current;
			size_t size = std::min(limit, len - current);
			CHECK_SUCCESS(LockMemory(chunk, size));
			CHECK_SUCCESS(UnlockMemory(chunk, size));
		}
	}
	
	return succeeded;
	#undef CHECK_SUCCESS
}

bool TouchMemoryRead(void* addr, size_t len)
{
	volatile uint8_t buffer;
	for (int offset = 0; offset < len; offset += PAGE_SIZE)
	{
		uint8_t* startOfPage = (uint8_t*)addr + offset;
		memcpy((uint8_t*)&buffer, startOfPage, sizeof(uint8_t));
	}
	
	return true;
}

void TouchMemoryManual(void* addr, size_t len, int prot)
{
	if ((prot & (PROT_READ | PROT_WRITE)))
	{
		volatile uint8_t buffer;
		for (int offset = 0; offset < len; offset += PAGE_SIZE)
		{
			uint8_t* start_of_page = (uint8_t*)addr + offset;
			memcpy((uint8_t*)&buffer, start_of_page, sizeof(uint8_t));
			
			if (prot & PROT_WRITE) {
				memcpy(start_of_page, (uint8_t*)&buffer, sizeof(uint8_t));
			}
		}
	}
}

bool TouchMemoryMadvise(void* addr, size_t len, int prot)
{
	if ((uint64_t)addr % PAGE_SIZE != 0 || len % PAGE_SIZE != 0)
	{
		// Fall back to manually touching the memory if it is not page-aligned (e.g. from malloc())
		TouchMemoryManual(addr, len, prot);
	}
	else if ((prot & (PROT_READ | PROT_WRITE)))
	{
		// Fault the memory in using madvise()
		int advice = (prot & PROT_WRITE) ? MADV_POPULATE_WRITE : MADV_POPULATE_READ;
		return (madvise(addr, len, advice) == 0);
	}

	return true;
}

extern "C" void* malloc(size_t size)
{
	static MallocFnPtr RealMalloc = GetRealFunction<MallocFnPtr>("malloc");
	void *ptr = RealMalloc(size);

	if (!ShimEnabled()) return ptr;

	if (ptr)
	{
		if (!TouchMemoryMadvise(ptr, size, PROT_READ | PROT_WRITE))
		{
			PrintError(
				#ifdef __i386__
				"failed to touch memory for malloc(%u) = %p, errno = %d\n",
				#else
				"failed to touch memory for malloc(%lu) = %p, errno = %d\n",
				#endif
				size, ptr, errno
			);
		}
	}

	return ptr;
}

extern "C" void* mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	static MmapFnPtr RealMmap = GetRealFunction<MmapFnPtr>("mmap");
	void *ptr = RealMmap(addr, len, prot, flags, fd, offset);

	if (!ShimEnabled()) return ptr;

	if
	(
		ptr != MAP_FAILED &&
		(flags & MAP_ANON || flags & MAP_ANONYMOUS) &&
		prot != PROT_NONE
	)
	{
		if (!TouchMemoryMadvise(ptr, len, prot))
		{
			PrintError(
				#ifdef __i386__
				"failed to touch memory for mmap(%p, %u, %d, %d, %d, %ld) = %p, errno = %d\n",
				#else
				"failed to touch memory for mmap(%p, %lu, %d, %d, %d, %ld) = %p, errno = %d\n",
				#endif
				addr, len, prot, flags, fd, offset, ptr, errno
			);
		}
	}

	return ptr;
}

extern "C" int mprotect(void* addr, size_t len, int prot)
{
	static MprotectFnPtr RealMprotect = GetRealFunction<MprotectFnPtr>("mprotect");
	int result = RealMprotect(addr, len, prot);

	if (!ShimEnabled()) return result;

	if (result == 0 && prot != PROT_NONE)
	{
		if (!TouchMemoryMadvise(addr, len, prot))
		{
			PrintError(
				#ifdef __i386__
				"failed to touch memory for mprotect(%p, %u, %d), errno = %d\n",
				#else
				"failed to touch memory for mprotect(%p, %lu, %d), errno = %d\n",
				#endif
				addr, len, prot, errno
			);
		}
	}

	return result;
}
