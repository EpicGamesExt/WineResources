#include "libmemory-patches.h"

#include <linux/magic.h>
#include <stdarg.h>
#include <stdio.h>
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

#include <sys/stat.h>
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

/* Win32 memory protection flags that grant writecopy access to a page */
static const uint32_t page_writecopy_flags =
    PAGE_EXECUTE_WRITECOPY |
    PAGE_WRITECOPY;

/* Used to selectively disable overcommit prevention for OOM handling */
static BOOL overcommit_prevention_disabled = FALSE;
static BOOL *overcommit_prevention_triggered = NULL;

/* Shared memory used to communicate between processes for selective overcommit prevention */
static const char *overcommit_prevention_shm_name = "overcommit_prevention_triggered";

/* Flags used to identify the Content Worker and Shader Compile Worker processes */
static BOOL is_content_worker_process = FALSE;
static BOOL is_shader_compile_worker_process = FALSE;


/***********************************************************************
 *           debug_mode_enabled
 *
 * Determines whether or not debug mode is enabled.
 */
static int debug_mode_enabled(void)
{
    static int debug_mode = -1;

    if (debug_mode == -1)
    {
        const char *env_var = getenv( "LIBMEMORY_PATCHES_DEBUG" );
        debug_mode = env_var && atoi(env_var);
    }

    return debug_mode;
}


/***********************************************************************
 *           get_cmdline
 *
 * Get the contents of /proc/self/cmdline with null byte separators converted
 * to spaces. You must call free() on the returned pointer when it is no longer
 * required.
 */
static char* get_cmdline(void)
{
    FILE *file = NULL;
    char *line = NULL;
    size_t len = 0;

    if (debug_mode_enabled())
        fprintf(stderr, "[libmemory-patches] get_cmdline() called.\n");

    /* Read /proc/self/cmdline which contains the executable's name */
    if(!(file = fopen("/proc/self/cmdline", "r")))
    {
        fprintf(stderr, "[libmemory-patches] ERROR: Failure in call to fopen() for '/proc/self/cmdline'\n");
        return NULL;
    }

    if (getline(&line, &len, file) == -1)
    {
        fprintf(stderr, "[libmemory-patches] ERROR: Failure in call to getline()\n");
        free(line);
        fclose(file);
        return NULL;
    }

    /* Replace null separator characters with space character */
    for (int i = 0; i < (len - 1); i++)
    {
        if (*(line + i) == '\0')
        {
            *(line + i) = ' ';
        }
    }

    fclose(file);
    return line;
}

/***********************************************************************
 *           init_overcommit_prevention_triggered
 *
 * Initialisation logic for setting up communication between processes for
 * selectively disabling overcommit prevention.
 */
static BOOL init_overcommit_prevention_triggered(void)
{
    if (debug_mode_enabled())
        fprintf(stderr, "[libmemory-patches] init_overcommit_prevention_triggered() called.\n");

    /* Check if we are 'FortniteContentWorker-Cmd.exe' or 'ShaderCompileWorker.exe' */
    char *cmdline = get_cmdline();
    if (cmdline != NULL)
    {
        if (strstr(cmdline, "FortniteContentWorker-Cmd.exe"))
        {
            is_content_worker_process = TRUE;
        }
        else if (strstr(cmdline, "ShaderCompileWorker.exe"))
        {
            is_shader_compile_worker_process = TRUE;
        }
    }
    free(cmdline);

    /* Initialise shared memory */
    int fd = shm_open(overcommit_prevention_shm_name, O_RDWR | O_CREAT, 0644);
    
    if (fd == -1)
    {
        fprintf(stderr, "[libmemory-patches] ERROR: Failure in call to shm_open()\n");
        return FALSE;
    }

    overcommit_prevention_triggered = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (overcommit_prevention_triggered == MAP_FAILED)
    {
        fprintf(stderr, "[libmemory-patches] ERROR: Failure in call to mmap()\n");
        overcommit_prevention_triggered = NULL;
        close(fd);
        return FALSE;
    }

    close(fd);
    return TRUE;
}

/***********************************************************************
 *           set_overcommit_prevention_triggered
 *
 * Sets overcommit_prevention_triggered to TRUE. Shared memory for inter-process
 * communication is initialised if required.
 */
static BOOL set_overcommit_prevention_triggered()
{
    if (debug_mode_enabled())
        fprintf(stderr, "[libmemory-patches] set_overcommit_prevention_triggered() called.\n");

    if (overcommit_prevention_triggered == NULL)
    {
        if (!init_overcommit_prevention_triggered())
        {
            fprintf(stderr, "[libmemory-patches] ERROR: Failed to initialise 'overcommit_prevention_triggered'\n");
            return FALSE;
        }
    }

    *overcommit_prevention_triggered = TRUE;
    return TRUE;
}

/***********************************************************************
 *           get_overcommit_prevention_triggered
 *
 * Retrieves the current value of overcommit_prevention_triggered. Shared
 * memory for inter-process communication is initialised if required.
 */
static BOOL get_overcommit_prevention_triggered(void)
{
    if (overcommit_prevention_triggered == NULL)
    {
        if (!init_overcommit_prevention_triggered())
        {
            fprintf(stderr, "[libmemory-patches] ERROR: Failed to initialise 'overcommit_prevention_triggered'\n");
            return FALSE;
        }
    }

    return *overcommit_prevention_triggered;
}

/***********************************************************************
 *           overcommit_prevention_exempted
 *
 * Determines whether the current process is exempt from overcommit prevention.
 */
int overcommit_prevention_exempted(void)
{
    static int exempted = -1;

    /* Initialise the static variable 'exempted' on the first call */
    if (exempted == -1)
    {
        if (debug_mode_enabled())
        fprintf(stderr, "[libmemory-patches] overcommit_prevention_exempted() called.\n");

        /* Define the list of processes which are to be exempted from overcommit prevention */
        const char *exempted_procs[] = {
            "CrashReportClient.exe",
            "CrashReportClientEditor.exe"
        };

        /* Read /proc/self/cmdline which contains the executable's name */
        char *cmdline = get_cmdline();

        if (cmdline != NULL)
        {
            /* Check if this process is in our exemption array */
            for (int i = 0; i < sizeof(exempted_procs) / sizeof(exempted_procs[0]); i++)
            {
                if (strstr(cmdline, exempted_procs[i]))
                {
                    exempted = TRUE;
                    break;
                }
            }
        }

        /* Clean up */
        free(cmdline);

        if (exempted == -1) exempted = FALSE;
    }

    return exempted;
}


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

    return prevent_overcommit && !overcommit_prevention_disabled;
}


/***********************************************************************
 *           should_prevent_overcommit
 *
 * Determines whether memory allocations should fail if insufficient memory
 * is available to satisfy a request.
 */
BOOL should_prevent_overcommit(void)
{
    if (is_content_worker_process && get_overcommit_prevention_triggered())
    {
        return FALSE;
    }

    return overcommit_prevention_enabled() && !overcommit_prevention_exempted();
}


/***********************************************************************
 *           overcommit_prevention_enabled
 *
 * Determines whether we should touch memory pages to ensure accurate memory
 * tracking for the purpose of preventing overcommit.
 */
BOOL should_touch_memory(void)
{
    if (is_content_worker_process && get_overcommit_prevention_triggered())
    {
        return FALSE;
    }

    return overcommit_prevention_enabled();
}


/***********************************************************************
 *           disable_overcommit_prevention
 *
 * Selectively disables overcommit prevention for the current process (and
 * sometimes the parent process) to allow for OOM handling. This function
 * should be called when a commit fails due to insufficient available memory.
 */
void disable_overcommit_prevention(void)
{
    if (debug_mode_enabled())
        fprintf(stderr, "[libmemory-patches] disable_overcommit_prevention() called.\n");

    /* Disable overcommit prevention for the current process */
    overcommit_prevention_disabled = TRUE;

    /* Disable overcommit prevention for 'FortniteContentWorker-Cmd.exe' */
    if(is_shader_compile_worker_process)
    {
        set_overcommit_prevention_triggered();
    }
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
 *           has_writecopy_flags
 *
 * Determines whether the specified memory protection flags grant writecopy access.
 */
BOOL has_writecopy_flags(uint32_t protect)
{
    return (protect & page_writecopy_flags);
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
