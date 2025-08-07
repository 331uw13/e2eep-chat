#ifndef EDITOR_LOG_H
#define EDITOR_LOG_H


enum LogInfoType : int {
    INFO = 0,
    WARNING,
    ERROR,
    FATAL
};


// IMPORTANT NOTE: Only 256 bytes for the buffer is reserved.
void _log_print_ext(
        enum LogInfoType type,
        const char* caller_func,
        const char* caller_func_file,
        const char* info_fmt, ...);

#define log_print(type, info, ...)\
    _log_print_ext(type, __func__, __FILE__, info, ##__VA_ARGS__)


// Contents of log file if it exists will be discarded by this function.
void assign_logfile(const char* filepath);
void close_logfile();

#endif
