#include <ruby.h>
#include <stdio.h>
#include <open62541.h>

typedef struct client_struct {
  UA_ClientConfig *config;
  UA_Client *client;
  UA_UInt16 default_ns;
} client_struct;

typedef struct node_struct {
  client_struct *client;
  UA_NodeId id;
} node_struct;
