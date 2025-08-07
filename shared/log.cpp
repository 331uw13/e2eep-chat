#include <cstdarg>
#include <cstring>
#include <cstdio>

#include <fcntl.h>
#include <unistd.h>

#include "log.hpp"

#define MAX_LOG_SIZE 256
#define MAX_LOG_FILEPATH 128

static const char* LOG_INFO_TYPE_STR[] = {
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};


struct ErrorLogGlobal {
    bool open;
    int  fd; // File descriptor.
}
static Log = {
    .open = false,
    .fd = -1
};


void assign_logfile(const char* filepath) {
    int permissions = S_IRUSR | S_IWUSR; // User has read and write permissions.

    Log.fd = open(filepath,
            O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, permissions);

    Log.open = (Log.fd >= 0);
}

void close_logfile() {
    close(Log.fd);
}

void _log_print_ext(
        enum LogInfoType type,
        const char* caller_func,
        const char* caller_func_file,
        const char* info_fmt, ...) {
    
    if(!Log.open) {
        return;
    }

    va_list args;
    va_start(args, info_fmt);

    char buf[MAX_LOG_SIZE] = { 0 };
    size_t psize = snprintf(buf, MAX_LOG_SIZE, 
            "%s | \"%s\" -> %s(): ", 
            LOG_INFO_TYPE_STR[type],
            caller_func_file,
            caller_func
            );

    size_t info_size = vsnprintf(buf + psize, MAX_LOG_SIZE, info_fmt, args);
    va_end(args);

    if(type == ERROR) {
        puts(buf);
    }

    write(Log.fd, buf, psize + info_size);
    write(Log.fd, "\n", 1);
}


