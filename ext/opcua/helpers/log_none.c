#include "log_none.h"

void UA_Log_None_log(void *_, UA_LogLevel level, UA_LogCategory category, const char *msg, va_list args) { }
void UA_Log_None_clear(void *logContext) { }
const UA_Logger UA_Log_None_ = {UA_Log_None_log, NULL, UA_Log_None_clear};
const UA_Logger *UA_Log_None = &UA_Log_None_;
