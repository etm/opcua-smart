#include <ruby.h>
#include <stdio.h>
#include <open62541.h>

typedef struct server_struct {
  UA_ServerConfig *config;
  UA_Server *server;
} server_struct;
