#include <ruby.h>
#include <stdio.h>
#include <open62541.h>
#include "../log_none.h"

typedef struct client_struct {
  UA_ClientConfig *config;
  UA_Client *master;
  UA_CreateSubscriptionRequest subscription_request;
  UA_CreateSubscriptionResponse subscription_response;

  UA_UInt32 subscription_interval;
  bool firstrun;
  bool started;
  bool debug;
  VALUE subs;
  bool subs_changed;
} client_struct;

typedef struct node_struct {
  client_struct *master;
  VALUE on_change;
  UA_NodeId id;
} node_struct;
