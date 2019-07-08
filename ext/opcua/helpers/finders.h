#ifndef __OPCUA_SMART_FINDERS_H__
#define __OPCUA_SMART_FINDERS_H__

#include <open62541.h>
#include <ruby.h>

RUBY_EXTERN UA_BrowsePathResult node_browse_path(UA_Server *server, UA_NodeId relative, UA_NodeId ref, UA_QualifiedName mqn, bool inverse);
RUBY_EXTERN bool node_get_reference(UA_Server *server, UA_NodeId parent, UA_NodeId *result);

#endif
