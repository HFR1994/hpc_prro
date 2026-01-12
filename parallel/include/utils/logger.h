#ifndef RRA_PARALLEL_LOGGER_H
#define RRA_PARALLEL_LOGGER_H

#include <stdio.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERR = 3
} LogLevel;

// Set the minimum log level (messages below this level won't be printed)
void log_set_level(LogLevel level);

// Get the current log level
LogLevel log_get_level(void);

// Logging functions
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warning(const char *format, ...);
void log_err(const char *format, ...);

// Optional: Enable/disable timestamps
void log_enable_timestamps(int enable);

#endif //RRA_PARALLEL_LOGGER_H
