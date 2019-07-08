#ifndef __OPCUA_SMART_LOG_NONE_H__
#define __OPCUA_SMART_LOG_NONE_H__

#include <open62541.h>
#include <ruby.h>

RUBY_EXTERN void UA_Log_None_log(void *_, UA_LogLevel level, UA_LogCategory category, const char *msg, va_list args);
RUBY_EXTERN void UA_Log_None_clear(void *logContext);
RUBY_EXTERN const UA_Logger UA_Log_None_;
RUBY_EXTERN const UA_Logger *UA_Log_None;

#endif
