#include "netagent/log.h"

#include <stdarg.h>
#include <stdio.h>

static LogLevel current_level = LOG_LEVEL_INFO;

static void log_message(
    LogLevel level,
    const char *prefix,
    const char *format,
    va_list args)
{
    if (level > current_level) {
        return;
    }

    FILE *stream =
        (level == LOG_LEVEL_ERROR) ? stderr : stdout;

    fprintf(stream, "[%s] ", prefix);
    vfprintf(stream, format, args);
    fputc('\n', stream);
}

void log_set_level(LogLevel level)
{
    current_level = level;
}

void log_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    log_message(
        LOG_LEVEL_ERROR,
        "ERROR",
        format,
        args
    );
    va_end(args);
}

void log_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    log_message(
        LOG_LEVEL_INFO,
        "INFO",
        format,
        args
    );
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    log_message(
        LOG_LEVEL_DEBUG,
        "DEBUG",
        format,
        args
    );
    va_end(args);
}