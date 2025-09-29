#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

log_level get_log_level(void)
{
    static int level = -1;

    if (level == -1)
    {
        const char *str = getenv( "LIBMEMORY_PATCHES_LOG_LEVEL" );

        if (str)
        {
            level = atoi(str);
            if (level != 0) return (log_level)level;
        }

        // Default value
        level = ERROR;
    }
    return (log_level)level;
}

void log_message(log_level level, const char* format, ...) {
    if (level >= get_log_level())
    {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }
}
