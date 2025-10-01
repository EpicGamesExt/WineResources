typedef enum
{
    TRACE = 1,
    INFO,
    WARNING,
    ERROR,
} log_level;

void log_message(log_level level, const char* format, ...);
