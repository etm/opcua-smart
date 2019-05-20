#include <ruby.h>
#include <ruby/intern.h>
#include <stdio.h>
#include <open62541.h>
#include <malloc.h>
#include "../log_none.h"
#include "../strnautocat.h"

typedef struct server_struct {
  UA_ServerConfig *config;
  UA_Server *server;
  UA_UInt16 default_ns;
} server_struct;

typedef struct node_struct {
  server_struct *server;
  UA_NodeId id;
} node_struct;
