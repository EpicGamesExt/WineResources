
#include "libmemory-patches.h"
#include "logger.h"

#include <libcgroup.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

BOOL use_cgroup_soft_memory_limit(void)
{
    static int use_soft_memory_limit = -1;

    if (use_soft_memory_limit == -1)
    {
        const char *str = getenv( "WINE_USE_CGROUP_SOFT_MEMORY_LIMIT" );

        use_soft_memory_limit = str && atoi(str) == 1;
    }
    return use_soft_memory_limit;
}

static BOOL get_cgroup_for_pid(pid_t pid, char *cgroup_name, unsigned int name_size)
{
    int ret = 0;
    void *handle = NULL;
    struct cgroup_file_info info;
    int base_level = 0;
    char controller[7]; /* "memory" */
    char *mount_point = NULL;
    pid_t *pids = NULL;
    int pids_size = 0;

    strcpy(controller, "memory");

    /* 
        While it is possible to get cgroup information from '/proc/<pid>/cgroup', this option is not
        always reliable with containers using cgroups v1 as the file entries can represent the
        cgroup names used on the host system (for example: "/docker/<container-id>/"). For this
        reason it is safer to perform a search using libcgroup to ensure compatibility with cgroups
        v1 and v2.
    */

    ret = cgroup_init();
    if (ret)
    {
        log_message(WARNING, "cgroup_init() failed: %s\n", cgroup_strerror(ret));
        return FALSE;
    }

    ret = cgroup_get_subsys_mount_point(controller, &mount_point);

    if (ret != 0)
    {
        log_message(WARNING, "cgroup_get_subsys_mount_point() failed: %s\n", cgroup_strerror(ret));
        return FALSE;
    }

    ret = cgroup_walk_tree_begin(controller, "/", 0, &handle, &info, &base_level);

    if (ret != 0)
    {
        log_message(WARNING, "cgroup_walk_tree_begin() failed: %s\n", cgroup_strerror(ret));
        return FALSE;
    }

    /* Iterate through each cgroup until we find the one that contains our PID */
    while (ret == 0)
    {
        if(info.type == CGROUP_FILE_TYPE_DIR)
        {
            int i = 0;
            const char *relative_path = info.full_path;

            /* Remove mount_point from the prefix of our path to get the name of the cgroup */
            if(strlen(info.full_path) > strlen(mount_point) && 
                (strncmp(info.full_path, mount_point, strlen(mount_point)) == 0))
            {
                relative_path += strlen(mount_point);
            }

            /* In some versions of libcgroup, cgroup_get_procs() requires a non-const string
               despite the fact that the contents of the string are not modified */
            if(cgroup_get_procs((char*)relative_path, controller, &pids, &pids_size))
            {
                log_message(WARNING, "cgroup_get_procs() failed for cgroup: '%s'\n", relative_path);
                ret = cgroup_walk_tree_next(0, &handle, &info, base_level);
                continue;
            }

            for(i = 0; i < pids_size; i++)
            {
                if (pids[i] == pid)
                {
                    snprintf(cgroup_name, name_size, "%s", relative_path);
                    cgroup_walk_tree_end(&handle);
                    return TRUE;
                }
            }
        }

        ret = cgroup_walk_tree_next(0, &handle, &info, base_level);
    }

    /* No cgroup was found for the given PID. */
    cgroup_walk_tree_end(&handle);
    return FALSE;
}

static BOOL init_cgroup_memory_info(struct cgroup_memory_info *cg_mem_info)
{
    const char *current_usage_memory_interface_v1 = "memory.usage_in_bytes";
    const char *swap_usage_memory_interface_v1 = "memory.memsw.usage_in_bytes";
    const char *hard_limit_memory_interface_v1 = "memory.limit_in_bytes";
    const char *soft_limit_memory_interface_v1 = "memory.soft_limit_in_bytes";
    const char *swap_limit_memory_interface_v1 = "memory.memsw.limit_in_bytes";

    const char *current_usage_memory_interface_v2 = "memory.current";
    const char *swap_usage_memory_interface_v2 = "memory.swap.current";
    const char *hard_limit_memory_interface_v2 = "memory.max";
    const char *soft_limit_memory_interface_v2 = "memory.low";
    const char *swap_limit_memory_interface_v2 = "memory.swap.max";

    struct cgroup_controller *group_controller = NULL;
    const char *cgroup_memory_controller_name = "memory";
    int ret = 0;
    struct cgroup *group = NULL;
    pid_t pid = getpid();
    FILE *fp = NULL;

    /* Initialise vars for reading memory.stat */
    struct cgroup_stat cg_stat = { 0 };
    void* handle = NULL;

    if ((fp = fopen("/proc/meminfo", "r")))
    {
        unsigned long long value = 0;
        char line[64];

        while (fgets(line, sizeof(line), fp))
        {
            if(sscanf(line, "MemTotal: %llu kB", &value) == 1)
            {
                cg_mem_info->cached_host_total_ram = value * 1024;
            }
            else if(sscanf(line, "SwapTotal: %llu kB", &value) == 1)
            {
                cg_mem_info->cached_host_total_swap = value * 1024;
            }
        }

        fclose(fp);
    }
    else
    {
        log_message(WARNING, "Failed to read /proc/meminfo\n");
        return FALSE;
    }

    /* Check which version of cgroups is in use: */
    /* https://rootlesscontaine.rs/getting-started/common/cgroup2/#checking-whether-cgroup-v2-is-already-enabled */
    if (access("/sys/fs/cgroup/cgroup.controllers", F_OK) == 0)
    {
        /* We are using cgroups v2 */
        cg_mem_info->version = 2;

        strcpy(cg_mem_info->current_usage_memory_interface,
            current_usage_memory_interface_v2);

        strcpy(cg_mem_info->swap_usage_memory_interface,
            swap_usage_memory_interface_v2);

        strcpy(cg_mem_info->hard_limit_memory_interface,
            hard_limit_memory_interface_v2);

        strcpy(cg_mem_info->soft_limit_memory_interface,
            soft_limit_memory_interface_v2);

        strcpy(cg_mem_info->swap_limit_memory_interface,
            swap_limit_memory_interface_v2);
    }
    else
    {
        /* Fall back to cgroups v1 */
        cg_mem_info->version = 1;

        strcpy(cg_mem_info->current_usage_memory_interface,
            current_usage_memory_interface_v1);

        strcpy(cg_mem_info->swap_usage_memory_interface,
            swap_usage_memory_interface_v1);

        strcpy(cg_mem_info->hard_limit_memory_interface,
            hard_limit_memory_interface_v1);

        strcpy(cg_mem_info->soft_limit_memory_interface,
            soft_limit_memory_interface_v1);

        strcpy(cg_mem_info->swap_limit_memory_interface,
            swap_limit_memory_interface_v1);
    }

    /* Find which cgroup the current process belongs to and
       get the relevant memory information for that group */
    if (!get_cgroup_for_pid(pid, cg_mem_info->cgroup_name, ARRAY_SIZE(cg_mem_info->cgroup_name)))
    {
        /* Failed to find a cgroup for the current process */
        log_message(WARNING, "failed to find a cgroup for the current process (PID: %d)\n", pid);
        return FALSE;
    }

    group = cgroup_new_cgroup(cg_mem_info->cgroup_name);
    if (group == NULL)
    {
        log_message(WARNING, "cgroup_new_cgroup() failed for cgroup '%s'\n", cg_mem_info->cgroup_name);
        return FALSE;
    }

    ret = cgroup_get_cgroup(group);
    if (ret != 0)
    {
        log_message(WARNING, "cgroup_get_cgroup() failed for cgroup '%s': %s\n", 
            cg_mem_info->cgroup_name,
            cgroup_strerror(ret));

        goto error;
    }

    group_controller = cgroup_get_controller(group, cgroup_memory_controller_name);
    if (group_controller == NULL)
    {
        log_message(WARNING, "cgroup_get_controller() failed for controller '%s' in cgroup '%s'\n",
            cgroup_memory_controller_name,
            cg_mem_info->cgroup_name);

        goto error;
    }

    /* Get current memory usage */
    ret = cgroup_get_value_uint64(
        group_controller,
        cg_mem_info->current_usage_memory_interface,
        &cg_mem_info->current_usage
    );

    if (ret != 0)
    {
        log_message(WARNING, "cgroup_get_value_uint64() failed for parameter '%s' in controller '%s': %s\n",
            cg_mem_info->current_usage_memory_interface,
            cgroup_memory_controller_name,
            cgroup_strerror(ret));

        cg_mem_info->current_usage = 0;
        goto error;
    }

    /* Get hard memory limit */
    ret = cgroup_get_value_uint64(
        group_controller,
        cg_mem_info->hard_limit_memory_interface,
        &cg_mem_info->hard_limit
    );

    if (ret != 0)
    {
        log_message(INFO, "No cgroup limit was detected for parameter '%s' in controller '%s': %s\n",
            cg_mem_info->hard_limit_memory_interface,
            cgroup_memory_controller_name,
            cgroup_strerror(ret));

        cg_mem_info->hard_limit = __UINT64_MAX__;
    }

    /* Get soft memory limit */
    ret = cgroup_get_value_uint64(
        group_controller,
        cg_mem_info->soft_limit_memory_interface,
        &cg_mem_info->soft_limit
    );

    if (ret != 0)
    {
        log_message(INFO, "No cgroup limit was detected for parameter '%s' in controller '%s': %s\n",
            cg_mem_info->soft_limit_memory_interface,
            cgroup_memory_controller_name,
            cgroup_strerror(ret));

        cg_mem_info->soft_limit = __UINT64_MAX__;
    }

    /* Get current swap usage */
    ret = cgroup_get_value_uint64(
        group_controller,
        cg_mem_info->swap_usage_memory_interface,
        &cg_mem_info->current_swap_usage
    );

    if (ret != 0)
    {
        log_message(WARNING, "cgroup_get_value_uint64() failed for parameter '%s' in controller '%s': %s\n",
            cg_mem_info->swap_usage_memory_interface,
            cgroup_memory_controller_name,
            cgroup_strerror(ret));

        cg_mem_info->current_swap_usage = 0;
        goto error;
    }

    /* Get swap memory limit */
    ret = cgroup_get_value_uint64(
        group_controller,
        cg_mem_info->swap_limit_memory_interface,
        &cg_mem_info->swap_limit
    );

    if (ret != 0)
    {
        log_message(INFO, "No cgroup limit was detected for parameter '%s' in controller '%s': %s\n",
            cg_mem_info->swap_limit_memory_interface,
            cgroup_memory_controller_name,
            cgroup_strerror(ret));

        cg_mem_info->swap_limit = __UINT64_MAX__;
    }

    /* Get reclaimable file pages from memory.stat */
    cg_mem_info->reclaimable_file_usage = 0;

    ret = cgroup_read_stats_begin(
        cgroup_memory_controller_name,
        cg_mem_info->cgroup_name,
        &handle,
        &cg_stat
    );

    if (ret != 0){
        log_message(WARNING, "cgroup_read_stats_begin() failed for memory.stat in controller '%s': %s\n",
            cgroup_memory_controller_name,
            cgroup_strerror(ret));
        goto error;
    }

    /* Read through memory.stat line by line until we find inactive_file and active_file */
    int found = 0;
    while(found < 2 && ret != ECGEOF)
    {
        if ((strcmp("inactive_file", cg_stat.name) == 0) ||
            (strcmp("active_file", cg_stat.name) == 0))
        {
            cg_mem_info->reclaimable_file_usage += strtoull(cg_stat.value, NULL, 10);
            found++;
        }

        ret = cgroup_read_stats_next(&handle, &cg_stat);

        if (ret != 0){
            log_message(WARNING, "cgroup_read_stats_next() failed for memory.stat in controller '%s': %s\n",
                cgroup_memory_controller_name,
                cgroup_strerror(ret));
            goto error;
        }
    }

    if (handle) cgroup_read_value_end(&handle);

    cgroup_free(&group);
    cg_mem_info->initialised = TRUE;
    return TRUE;

error:
    cgroup_free(&group);
    if (handle) cgroup_read_value_end(&handle);
    return FALSE;
}

static BOOL update_cgroup_memory_info(struct cgroup_memory_info *cg_mem_info)
{
    const char *cgroup_memory_controller_name = "memory";
    void *handle = NULL;
    char buffer[256];
    int ret = 0;
    struct cgroup_stat cg_stat = { 0 };

    /* Update current memory usage */
    ret = cgroup_read_value_begin(
        cgroup_memory_controller_name,
        cg_mem_info->cgroup_name,
        cg_mem_info->current_usage_memory_interface,
        &handle,
        buffer,
        ARRAY_SIZE(buffer)
    );

    if (ret == 0)
    {
        cg_mem_info->current_usage = strtoull(buffer, NULL, 10);
    }
    else
    {
        log_message(ERROR, "cgroup_read_value_begin() failed for parameter '%s' in controller '%s': %s\n",
                cg_mem_info->current_usage_memory_interface,
                cgroup_memory_controller_name,
                cgroup_strerror(ret));
        return FALSE;
    }

    if (handle) cgroup_read_value_end(&handle);

    /* Update current swap usage */
    ret = cgroup_read_value_begin(
        cgroup_memory_controller_name,
        cg_mem_info->cgroup_name,
        cg_mem_info->swap_usage_memory_interface,
        &handle,
        buffer,
        ARRAY_SIZE(buffer)
    );

    if (ret == 0)
    {
        cg_mem_info->current_swap_usage = strtoull(buffer, NULL, 10);
    }
    else
    {
        log_message(ERROR, "cgroup_read_value_begin() failed for parameter '%s' in controller '%s': %s\n",
                cg_mem_info->swap_usage_memory_interface,
                cgroup_memory_controller_name,
                cgroup_strerror(ret));
        return FALSE;
    }

    if (handle) cgroup_read_value_end(&handle);

    /* Update reclaimable file memory usage */
    cg_mem_info->reclaimable_file_usage = 0;

    ret = cgroup_read_stats_begin(
        cgroup_memory_controller_name,
        cg_mem_info->cgroup_name,
        &handle,
        &cg_stat
    );

    if (ret != 0){
        log_message(ERROR, "cgroup_read_stats_begin() failed for memory.stat in controller '%s': %s\n",
            cgroup_memory_controller_name,
            cgroup_strerror(ret));
        return FALSE;
    }

    /* Read through memory.stat line by line until we find inactive_file and active_file */
    int found = 0;
    while(found < 2 && ret != ECGEOF)
    {
        if ((strcmp("inactive_file", cg_stat.name) == 0) ||
            (strcmp("active_file", cg_stat.name) == 0))
        {
            cg_mem_info->reclaimable_file_usage += strtoull(cg_stat.value, NULL, 10);
            found++;
        }

        ret = cgroup_read_stats_next(&handle, &cg_stat);

        if (ret != 0){
            log_message(ERROR, "cgroup_read_stats_next() failed for memory.stat in controller '%s': %s\n",
                cgroup_memory_controller_name,
                cgroup_strerror(ret));
            return FALSE;
        }
    }

    if (handle) cgroup_read_value_end(&handle);

    return TRUE;
}

struct cgroup_memory_info *get_cgroup_memory_info(void)
{
    static struct cgroup_memory_info cg_mem_info = { 0 };
    static BOOL init_called = FALSE;

    // Only attempt init_cgroup_memory_info() once
    if (init_called && !cg_mem_info.initialised) return NULL;

    if (!cg_mem_info.initialised)
    {
        init_called = TRUE;
        if (!init_cgroup_memory_info(&cg_mem_info))
        {
            log_message(WARNING, "cgroup_memory_info was not initialised\n");
            return NULL;
        }
    }
    else
    {
        if(!update_cgroup_memory_info(&cg_mem_info))
        {
            log_message(ERROR, "failed to update cgroup_memory_info\n");
        }
    }

    return &cg_mem_info;
}

struct current_memory_info get_current_memory_info(void)
{
    struct current_memory_info current_mem_info = { 0 };

    u_int64_t memory_limit = 0;
    u_int64_t current_usage = 0;

    struct cgroup_memory_info *cg_mem_info = get_cgroup_memory_info();

    if(cg_mem_info &&
        use_cgroup_soft_memory_limit() &&
        cg_mem_info->soft_limit < cg_mem_info->hard_limit &&
        cg_mem_info->soft_limit > 0)
    {
        memory_limit = cg_mem_info->soft_limit;
    }
    else if (cg_mem_info)
    {
        memory_limit = cg_mem_info->hard_limit;
    }

    if (cg_mem_info && cg_mem_info->initialised && 
        memory_limit > 0 && memory_limit < cg_mem_info->cached_host_total_ram)
    {
        /* The operation succeeded, use cgroup memory values */

        current_mem_info.totalram = memory_limit;
        current_mem_info.totalswap = cg_mem_info->cached_host_total_swap;

        /* Calculate current memory usage, excluding reclaimable file pages */
        if (cg_mem_info->current_usage > cg_mem_info->reclaimable_file_usage)
        {
            current_usage = cg_mem_info->current_usage - cg_mem_info->reclaimable_file_usage;
        }
        else
        {
            current_usage = 0;
        }

        /* Calculate free ram based on current memory usage */
        if (current_usage >= memory_limit)
        {
            current_mem_info.freeram = 0;
        }
        else
        {
            current_mem_info.freeram = memory_limit - current_usage;
        }

        if(cg_mem_info->version == 1)
        {
            /* For cgroups v1:
                - swap_limit = memory limit + swap limit
                - current_swap_usage = memory usage + swap usage */
            u_int64_t real_swap_usage = 0;

            if (current_mem_info.totalswap > cg_mem_info->swap_limit - cg_mem_info->hard_limit &&
                cg_mem_info->hard_limit < cg_mem_info->cached_host_total_ram)
            {
                current_mem_info.totalswap = cg_mem_info->swap_limit - cg_mem_info->hard_limit;
            }

            /* current_swap_usage and current_usage are not in perfect sync
                so we must perform this check to guard against uint overflow */
            if (cg_mem_info->current_swap_usage >= cg_mem_info->current_usage)
            {
                real_swap_usage = cg_mem_info->current_swap_usage - cg_mem_info->current_usage;
            }

            if (current_mem_info.totalswap >= real_swap_usage)
            {
                current_mem_info.freeswap = current_mem_info.totalswap - real_swap_usage;
            }
            else
            {
                current_mem_info.freeswap = 0;
            }
        }
        else
        {
            if (current_mem_info.totalswap > cg_mem_info->swap_limit)
            {
                current_mem_info.totalswap = cg_mem_info->swap_limit;
            }

            if (current_mem_info.totalswap >= cg_mem_info->current_swap_usage)
            {
                current_mem_info.freeswap = current_mem_info.totalswap - cg_mem_info->current_swap_usage;
            }
            else
            {
                current_mem_info.freeswap = 0;
            }
        }
    }
    else
    {
        FILE *fp;

        if ((fp = fopen("/proc/meminfo", "r")))
        {
            unsigned long long value, mem_available = 0;
            char line[64];

            while (fgets(line, sizeof(line), fp))
            {
                if(sscanf(line, "MemTotal: %llu kB", &value) == 1)
                    current_mem_info.totalram += value * 1024;
                else if(sscanf(line, "MemFree: %llu kB", &value) == 1)
                    current_mem_info.freeram += value * 1024;
                else if(sscanf(line, "SwapTotal: %llu kB", &value) == 1)
                    current_mem_info.totalswap += value * 1024;
                else if(sscanf(line, "SwapFree: %llu kB", &value) == 1)
                    current_mem_info.freeswap += value * 1024;
                else if (sscanf(line, "Buffers: %llu", &value))
                    current_mem_info.freeram += value * 1024;
                else if (sscanf(line, "Cached: %llu", &value))
                    current_mem_info.freeram += value * 1024;
                else if (sscanf(line, "MemAvailable: %llu", &value))
                    mem_available = value * 1024;
            }
            fclose(fp);
            if (mem_available) current_mem_info.freeram = mem_available;
        }
    }

    return current_mem_info;
}