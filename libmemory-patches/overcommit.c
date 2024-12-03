#include "libmemory-patches.h"

#include <linux/magic.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <fcntl.h>

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <sys/mman.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>

// Copied from Wine's dlls/ntdll/unix/unix_private.h
static const SIZE_T page_size = 0x1000;

/* Win32 memory protection flags that grant write access to a page */
static const uint32_t page_write_flags =
    PAGE_EXECUTE_READWRITE |
    PAGE_EXECUTE_WRITECOPY |
    PAGE_READWRITE |
    PAGE_WRITECOPY;


/***********************************************************************
 *           overcommit_prevention_enabled
 *
 * Determines whether we will attempt to prevent memory from being overcommitted.
 */
int overcommit_prevention_enabled(void)
{
    static int prevent_overcommit = -1;

    if (prevent_overcommit == -1)
    {
        const char *env_var = getenv( "WINE_PREVENT_OVERCOMMIT" );
        prevent_overcommit = env_var && atoi(env_var);
    }

    return prevent_overcommit;
}


/***********************************************************************
 *           overcommit_use_madvise
 *
 * Determines whether touch_committed_pages() will use madvise() to fault
 * in newly-committed pages.
 */
int overcommit_use_madvise(void)
{
    static int use_madvise = -1;

    if (use_madvise == -1)
    {
        const char *env_var = getenv( "WINE_OVERCOMMIT_USE_MADVISE" );
        use_madvise = env_var && atoi(env_var);
    }

    return use_madvise;
}


/***********************************************************************
 *           memory_available_for_commit
 *
 * Helper function that determines whether enough memory is available to
 * satisfy a given page commit request.
 */
BOOL memory_available_for_commit(size_t size)
{
    struct current_memory_info current_mem_values = get_current_memory_info();
    uint64_t TotalCommitLimit    = (current_mem_values.totalram + current_mem_values.totalswap);
    uint64_t TotalCommittedPages = (
        current_mem_values.totalram + current_mem_values.totalswap -
        current_mem_values.freeram - current_mem_values.freeswap
    );
    
    if (TotalCommitLimit - TotalCommittedPages <= size)
        return FALSE;
    
    return TRUE;
}


/***********************************************************************
 *           touch_committed_pages
 *
 * Helper function that accesses committed pages to ensure they are backed
 * by physical memory and will be counted correctly when calculating available
 * memory for overcommit prevention.
 */
void touch_committed_pages(void* base, size_t size, uint32_t protect)
{
    uint32_t read_flags =
        page_write_flags |
        PAGE_EXECUTE_READ |
        PAGE_READONLY;

    uint32_t exclude_flags = PAGE_GUARD;

    if ((protect & read_flags) && !(protect & exclude_flags))
    {
        // If madvise() is enabled the use it
        if (overcommit_use_madvise())
        {
            int advice = has_write_flags(protect) ? MADV_POPULATE_WRITE : MADV_POPULATE_READ;
            if (madvise(base, size, advice) == 0) {
                return;
            }
        }
        
        // Fall back to manually touching the pages if madvise() is disabled or if it fails
        volatile BYTE buffer;
        for (int offset = 0; offset < size; offset += page_size)
        {
            BYTE* start_of_page = (BYTE*)base + offset;
            memcpy((BYTE *)&buffer, start_of_page, sizeof(BYTE));

            if (has_write_flags(protect)) {
                memcpy(start_of_page, (BYTE*)&buffer, sizeof(BYTE));
            }
        }
    }
}


/***********************************************************************
 *           has_write_flags
 *
 * Determines whether the specified memory protection flags grant write access.
 */
BOOL has_write_flags(uint32_t protect)
{
    return (protect & page_write_flags);
}


/***********************************************************************
 *           is_memory_backed_file
 *
 * Determines whether the specified file is backed by memory.
 */
BOOL is_memory_backed_file(int fd)
{
    // Determine whether the file is an anyonymous file created with memfd_create()
    if (fcntl(fd, F_GET_SEALS) != -1) {
        return TRUE;
    }
    
    // Determine whether the file is stored under a memory-backed filesystem (ramfs or tmpfs)
    struct statfs stats;
    if (fstatfs(fd, &stats) == 0 && (stats.f_type == RAMFS_MAGIC || stats.f_type == TMPFS_MAGIC)) {
        return TRUE;
    }
    
    return FALSE;
}
