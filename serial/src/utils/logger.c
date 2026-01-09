#include "utils/logger.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

static LogLevel current_log_level = LOG_LEVEL_DEBUG;
static int timestamps_enabled = 0;

static const char *level_strings[] = {
    "DEBUG",
    "LOG",
    "WARNING",
    "ERROR"
};

static const char *level_colors[] = {
    "\033[36m",  // Cyan for DEBUG
    "\033[37m",  // White for LOG
    "\033[33m",  // Yellow for WARNING
    "\033[31m"   // Red for ERROR
};

static const char *color_reset = "\033[0m";

void log_set_level(LogLevel level) {
    current_log_level = level;
}

LogLevel log_get_level(void) {
    return current_log_level;
}

void log_enable_timestamps(int enable) {
    timestamps_enabled = enable;
}

static void log_message(LogLevel level, const char *format, va_list args) {
    if (level < current_log_level) {
        return;
    }

    FILE *stream = (level >= LOG_LEVEL_ERR) ? stderr : stdout;

    // Print timestamp if enabled
    if (timestamps_enabled) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_buffer[20];
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(stream, "[%s] ", time_buffer);
    }

    // Print log level with color
    fprintf(stream, "%s[%s]%s ", level_colors[level], level_strings[level], color_reset);

    // Print the actual message
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_LOG, format, args);
    va_end(args);
}

void log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_WARNING, format, args);
    va_end(args);
}

void log_err(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_ERR, format, args);
    va_end(args);
}
