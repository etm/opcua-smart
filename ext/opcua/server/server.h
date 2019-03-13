#include <ruby.h>
#include <stdio.h>
#include <open62541.h>

RUBY_EXTERN VALUE cNodeId;

typedef struct server_struct {
  UA_ServerConfig *config;
  UA_Server *server;
  VALUE nodes;
} server_struct;

typedef struct nodeid_struct {
  UA_NodeId id;
  VALUE name;
} nodeid_struct;
