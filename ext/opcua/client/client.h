#include <ruby.h>
#include <stdio.h>
#include <open62541.h>
#include "log_none.h"
#include "finders.h"

typedef struct client_struct {
  UA_ClientConfig *config;
  UA_Client *master;
  UA_CreateSubscriptionRequest subscription_request;
  UA_CreateSubscriptionResponse subscription_response;

  UA_UInt32 subscription_interval;
  bool subrun;
  bool started;
  bool debug;
  VALUE subs;
  UA_UInt16 default_ns;
} client_struct;

typedef struct node_struct {
  client_struct *master;
  VALUE on_change;
  UA_NodeId id;
  UA_UInt32 monId;
  int waiting;
} node_struct;
