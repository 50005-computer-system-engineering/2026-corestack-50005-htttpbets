#ifndef LIBLOG_H
#define LIBLOG_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

static const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

#define LOG(level, fmt, ...) fprintf(stderr, "[%s] %s:%d: " fmt "\n", level_str[level], __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_D(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) LOG(LOG_INFO,  fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LOG(LOG_WARN,  fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) LOG(LOG_ERROR, fmt, ##__VA_ARGS__)

#endif