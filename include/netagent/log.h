#ifndef NETAGENT_LOG_H
#define NETAGENT_LOG_H

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} LogLevel;

void log_set_level(LogLevel level);

void log_error(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);

#endif