#ifndef THORNYREADER_LOG_H
#define THORNYREADER_LOG_H

#include <cstdarg>

class CRLog
{
public:
    enum LogLevel {
        FATAL,
        ERROR,
        WARN,
        INFO,
        DEBUG,
        TRACE
    };
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    static bool levelEnabled(LogLevel level);
    static void fatal(const char* msg, ...);
    static void error(const char* msg, ...);
    static void warn(const char* msg, ...);
    static void info(const char* msg, ...);
    static void debug(const char* msg, ...);
    static void trace(const char* msg, ...);
private:
    CRLog();
    ~CRLog();
};

/// fatal error function calls fatal error handler
void crFatalError( int code, const char * errorText );
inline void crFatalError() { crFatalError( -1, "Unknown fatal error" ); }

#endif //THORNYREADER_LOG_H
