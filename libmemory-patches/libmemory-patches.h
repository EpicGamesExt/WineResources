#include <stdint.h>
#include <stdio.h>

typedef int BOOL;

struct cgroup_memory_info
{
    BOOL initialised;
    int version;
    char cgroup_name[FILENAME_MAX];
    /* Value: "memory.usage_in_bytes" or "memory.current" */
    char current_usage_memory_interface[22];
    /* Value: "memory.memsw.usage_in_bytes" or "memory.swap.current" */
    char swap_usage_memory_interface[28];
    /* Value: "memory.limit_in_bytes" or "memory.max" */
    char hard_limit_memory_interface[22];
    /* Value: "memory.soft_limit_in_bytes" or "memory.low" */
    char soft_limit_memory_interface[27];
    /* Value: "memory.memsw.limit_in_bytes" or "memory.swap.max" */
    char swap_limit_memory_interface[28];
    uint64_t current_usage;
    uint64_t hard_limit;
    uint64_t soft_limit;
    uint64_t current_swap_usage;
    uint64_t swap_limit;
    uint64_t reclaimable_file_usage;
    unsigned long long cached_host_total_ram;
    unsigned long long cached_host_total_swap;
};

struct current_memory_info
{
    unsigned long long totalram;
    unsigned long long freeram;
    unsigned long long totalswap;
    unsigned long long freeswap;
};

#define LIBMEMORY_PATCHES_API __attribute__((visibility ("default")))

LIBMEMORY_PATCHES_API extern struct current_memory_info get_current_memory_info(void);
LIBMEMORY_PATCHES_API int overcommit_prevention_enabled(void);
LIBMEMORY_PATCHES_API int overcommit_prevention_exempt(void);
LIBMEMORY_PATCHES_API BOOL memory_available_for_commit(size_t size);
LIBMEMORY_PATCHES_API void touch_committed_pages(void* base, size_t size, uint32_t protect);
LIBMEMORY_PATCHES_API BOOL has_write_flags(uint32_t protect);
LIBMEMORY_PATCHES_API BOOL has_writecopy_flags(uint32_t protect);
LIBMEMORY_PATCHES_API BOOL is_memory_backed_file(int fd);
