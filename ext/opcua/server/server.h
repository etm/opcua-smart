#include <ruby.h>
#include <ruby/intern.h>
#include <stdio.h>
#include <open62541.h>
#include <malloc.h>
#include "../log_none.h"
#include "../strnautocat.h"

typedef struct server_struct {
  UA_ServerConfig *config;
  UA_Server *master;
  UA_UInt16 default_ns;
  bool debug;
} server_struct;

typedef struct node_struct {
  server_struct *master;
  UA_NodeId id;
} node_struct;
