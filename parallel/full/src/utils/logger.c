#include "utils/logger.h"
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <omp.h>

static LogLevel current_log_level = LOG_LEVEL_DEBUG;
static int current_world_rank = INT_MIN;
static int timestamps_enabled = 0;

static const char *level_strings[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "MAIN"
};

static const char *level_colors[] = {
    "\033[36m",  // Cyan for DEBUG
    "\033[37m",  // White for LOG
    "\033[33m",  // Yellow for WARNING
    "\033[31m",  // Red for ERROR
    "\033[34m"   // Blue for MAIN Process
};

static const char *color_reset = "\033[0m";

void log_set_level(LogLevel level) {
    current_log_level = level;
}

LogLevel log_get_level(void) {
    return current_log_level;
}

void log_enable_timestamps(const int enable) {
    timestamps_enabled = enable;
}

void log_world_rank(const int world_rank) {
    current_world_rank = world_rank;
}

static void log_message(LogLevel level, const char *format, va_list args) {
    // Rule 1: MAIN prints only on rank 0
    if (level == 100 && current_world_rank != 0) {
        return;
    }

    // Rule 2: Normal log filtering
    if (level < current_log_level) {
        return;
    }

    //5 element is the MAIN RANK logger process
    level = (level == 100) ? 4:level;
    FILE *stream = (level == LOG_LEVEL_ERR) ? stderr : stdout;

    if (format[0] != '\0') {
        // Print timestamp if enabled
        if (timestamps_enabled) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char time_buffer[20];
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            fprintf(stream, "[%s] ", time_buffer);
        }

        // Print log level with color
        const int thread_num = omp_in_parallel() ? omp_get_thread_num() : INT_MIN;

        if (thread_num == INT_MIN) {
          fprintf(stream, "%s[%s:%d]%s ", level_colors[level], level_strings[level], current_world_rank, color_reset);
        } else {
          fprintf(stream, "%s[%s:%d.%d]%s ", level_colors[level], level_strings[level], current_world_rank, thread_num, color_reset);
        }

        // Print the actual message
        vfprintf(stream, format, args);
    }

    fprintf(stream, "\n");
    fflush(stream);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_INFO, format, args);
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

void log_main(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(100, format, args);
    va_end(args);
}