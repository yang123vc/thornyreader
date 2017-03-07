#include "trlog.h"
#include <android/log.h>
#include "thornyreader.h"

static CRLog::LogLevel log_level_ = CRLog::TRACE;

CRLog::CRLog() {}

CRLog::~CRLog() {}

void CRLog::setLevel(CRLog::LogLevel level)
{
    log_level_ = level;
}

CRLog::LogLevel CRLog::getLevel()
{
    return log_level_;
}

bool CRLog::levelEnabled(CRLog::LogLevel level)
{
    return log_level_ >= level;
}

void CRLog::fatal(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_FATAL, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}

void CRLog::error(const char* msg, ...)
{
    if (log_level_ < ERROR) {
        return;
    }
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_ERROR, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}

void CRLog::warn(const char* msg, ...)
{
    if (log_level_ < WARN) {
        return;
    }
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_WARN, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}

void CRLog::info(const char* msg, ...)
{
    if (log_level_ < INFO) {
        return;
    }
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_INFO, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}

void CRLog::debug(const char* msg, ...)
{
    if (log_level_ < DEBUG) {
        return;
    }
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_DEBUG, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}

void CRLog::trace(const char* msg, ...)
{
    if (log_level_ < TRACE) {
        return;
    }
    va_list args;
    va_start(args, msg);
    __android_log_vprint(ANDROID_LOG_VERBOSE, THORNYREADER_LOG_TAG, msg, args);
    va_end(args);
}