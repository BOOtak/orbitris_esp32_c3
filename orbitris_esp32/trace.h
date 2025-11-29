#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

constexpr auto TRACE_BUFFER_SIZE = 1024;

void do_print(const char *buffer);

static inline void trace(const char *fmt, ...)
{
    // Define a sufficiently large buffer for your trace messages.
    // 1024 bytes should be adequate for most general tracing.
    char buffer[TRACE_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, TRACE_BUFFER_SIZE, fmt, args);
    va_end(args);

    do_print(buffer);
}