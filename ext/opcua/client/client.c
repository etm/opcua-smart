#include "client.h"
#include "../../../cert/cert.h"
#include "../../../cert/cert_key.h"

VALUE mOPCUA = Qnil;
VALUE cClient = Qnil;
VALUE cNode = Qnil;

/* -- */
static void  node_free(node_struct *ns) { //{{{
  if (ns != NULL) {
    rb_gc_unregister_address(&ns->on_change);
    free(ns);
  }
} //}}}
static VALUE node_alloc(VALUE klass, client_struct *client, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->client = client;
  ns->id  = nodeid;
  ns->on_change  = Qnil;

	return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */
static VALUE node_value(VALUE self) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->client->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Client_readValueAttribute(ns->client->client, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  return rb_ary_entry(ret,0);
} //}}}
static VALUE node_on_change(VALUE self) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->client->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "you need to supply a block with #on_change");

  ns->on_change = rb_block_proc();
  rb_gc_register_address(&ns->on_change);

  rb_ary_push(ns->client->subs,self);
  ns->client->subs_changed = true;

  return self;
} //}}}

/* -- */
static void  client_free(client_struct *pss) { //{{{
  if (pss != NULL) {
    if (!pss->firstrun) {
      // do we need to delete the individual monResponse (#UA_MonitoredItemCreateResult_clear)?
      UA_Client_Subscriptions_deleteSingle(pss->client,pss->subscription_response.subscriptionId);
    }
    UA_Client_disconnect(pss->client);
    UA_Client_delete(pss->client);
    free(pss);
  }
} //}}}
static VALUE client_alloc(VALUE self) { //{{{
  client_struct *pss;
  pss = (client_struct *)malloc(sizeof(client_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA client.");

  pss->client = UA_Client_new();
  pss->config = UA_Client_getConfig(pss->client);
  pss->firstrun = true;
  pss->started = true;
  pss->debug = true;
  pss->subs = rb_ary_new();
  pss->subs_changed = false;
  pss->subscription_interval = 500;

  UA_CreateSubscriptionRequest_init(&pss->subscription_request);
  pss->subscription_request.requestedPublishingInterval = pss->subscription_interval;
  pss->subscription_request.requestedLifetimeCount = 10000;
  pss->subscription_request.requestedMaxKeepAliveCount = 10;
  pss->subscription_request.maxNotificationsPerPublish = 0;
  pss->subscription_request.publishingEnabled = true;
  pss->subscription_request.priority = 0;

	return Data_Wrap_Struct(self, NULL, client_free, pss);
} //}}}

/* ++ */ //}}}
static VALUE client_init(VALUE self,VALUE url,VALUE user,VALUE pass) { //{{{
  client_struct *pss;
  UA_StatusCode retval;

  Data_Get_Struct(self, client_struct, pss);

  VALUE str = rb_obj_as_string(url);
  if (NIL_P(str) || TYPE(str) != T_STRING) {
    pss->started = false;
    rb_raise(rb_eTypeError, "cannot convert url to string");
  }
  char *nstr = (char *)StringValuePtr(str);

  UA_ByteString certificate;
                certificate.data = (UA_Byte *)cert_der;
                certificate.length  = cert_der_len;
  UA_ByteString privateKey;
                privateKey.data = (UA_Byte *)cert_key_der;
                privateKey.length = cert_key_der_len;

  /* Load the trustList. Load revocationList is not supported now */
  size_t trustListSize = 0;
  UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

  UA_ByteString *revocationList = NULL;
  size_t revocationListSize = 0;

  UA_ClientConfig_setDefaultEncryption(pss->config, certificate, privateKey,
                                       trustList, trustListSize,
                                       revocationList, revocationListSize);

  for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
      UA_ByteString_clear(&trustList[deleteCount]);
  }

  pss->config->securityMode = UA_MESSAGESECURITYMODE_NONE; /* require no encryption, but tokens might be still enc'd */

  //{{{
  // UA_ClientConfig_setDefault(pss->config);
  // /* Listing endpoints */
  // UA_EndpointDescription* endpointArray = NULL;
  // size_t endpointArraySize = 0;
  // retval = UA_Client_getEndpoints(pss->client, "opc.tcp://localhost:4840",
  //                                               &endpointArraySize, &endpointArray);
  // if(retval != UA_STATUSCODE_GOOD) {
  //     UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  //     UA_Client_delete(pss->client);
  //     return EXIT_FAILURE;
  // }
  // printf("%i endpoints found\n", (int)endpointArraySize);

  // for(size_t i=0;i<endpointArraySize;i++) {
  //   printf("URL of endpoint %i is %.*s\n", (int)i,
  //          (int)endpointArray[i].endpointUrl.length,
  //          endpointArray[i].endpointUrl.data);

  //   printf("\tuserIdentityTokens\n");
  //   for(size_t j=0;j<endpointArray[i].userIdentityTokensSize;++j) {
  //     printf("\t\t");
  //     switch(endpointArray[i].userIdentityTokens[j].tokenType) {
  //       case UA_USERTOKENTYPE_ANONYMOUS:
  //         printf("Anonymous");
  //         break;
  //       case UA_USERTOKENTYPE_USERNAME:
  //         printf("Username");
  //         break;
  //       case UA_USERTOKENTYPE_CERTIFICATE:
  //         printf("Certificate");
  //         break;
  //       case UA_USERTOKENTYPE_ISSUEDTOKEN:
  //         printf("Issued Token");
  //         break;
  //       default:
  //         printf("No valid token type");
  //         break;
  //     }
  //     printf("(%s)\n",endpointArray[i].userIdentityTokens[j].securityPolicyUri.data);
  //   }
  // }
  // UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  //}}}

  if (NIL_P(user) || NIL_P(pass)) {
    retval = UA_Client_connect(pss->client,nstr);
  } else {
    VALUE vustr = rb_obj_as_string(user);
    if (NIL_P(vustr) || TYPE(vustr) != T_STRING) {
      pss->started = false;
      rb_raise(rb_eTypeError, "cannot convert user to string");
    }
    char *ustr = (char *)StringValuePtr(vustr);

    VALUE vpstr = rb_obj_as_string(pass);
    if (NIL_P(vpstr) || TYPE(vpstr) != T_STRING) {
      pss->started = false;
      rb_raise(rb_eTypeError, "cannot convert user to string");
    }
    char *pstr = (char *)StringValuePtr(vpstr);

    retval = UA_Client_connect_username(pss->client, nstr, ustr, pstr);
  }
  if (retval != UA_STATUSCODE_GOOD) {
    pss->started = false;
    rb_raise(rb_eRuntimeError, "Client could not be started.");
  }

  return self;
} //}}}
static VALUE client_get(VALUE self,VALUE ns,VALUE id) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (NIL_P(ns) || TYPE(ns) != T_FIXNUM)
    rb_raise(rb_eTypeError, "ns is not a valid (numeric) namespace id");
  if (TYPE(id) == T_FIXNUM) {
    return node_alloc(cNode, pss, UA_NODEID_NUMERIC(NUM2INT(ns), NUM2INT(id)));
  } else {
    VALUE str = rb_obj_as_string(id);
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert url to string");
    char *nstr = (char *)StringValuePtr(str);

    return node_alloc(cNode, pss, UA_NODEID_STRING(NUM2INT(ns), nstr));
  }
} //}}}
static VALUE client_subscription_interval(VALUE self) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  return UINT2NUM(pss->subscription_request.requestedPublishingInterval);
} //}}}
static VALUE client_subscription_interval_set(VALUE self, VALUE val) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (NIL_P(val) || TYPE(val) != T_FIXNUM)
    rb_raise(rb_eTypeError, "subscription interval is not an integer");

  pss->subscription_request.requestedPublishingInterval = NUM2UINT(val);
  return self;
} //}}}
static VALUE client_debug(VALUE self) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  return (pss->debug) ? Qtrue : Qfalse;
} //}}}
static VALUE client_debug_set(VALUE self, VALUE val) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (val == Qtrue) {
    pss->config->logger = UA_Log_Stdout_;
    pss->debug = Qtrue;
  } else {
    pss->config->logger = UA_Log_None_;
    pss->debug = Qfalse;
  }
  return self;
} //}}}

static void  client_run_handler(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId, void *monContext, UA_DataValue *value) { //{{{
  VALUE val = (VALUE)monContext;

  if (NIL_P(val) || TYPE(val) != T_NIL) {
    VALUE args = rb_ary_new2(3);
    VALUE ret = extract_value(value->value);
    rb_ary_store(args,0,rb_ary_entry(ret,0));
    if (value->hasSourceTimestamp) {
      rb_ary_store(args,1,rb_time_new(UA_DateTime_toUnixTime(value->sourceTimestamp),0));
    } else {
      if (value->hasServerTimestamp) {
        rb_ary_store(args,1,rb_time_new(UA_DateTime_toUnixTime(value->serverTimestamp),0));
      } else {
        rb_ary_store(args,1,Qnil);
      }
    }
    rb_ary_store(args,2,rb_ary_entry(ret,1));
    rb_proc_call(val,args);
  }
} //}}}
static void  client_run_iterate(VALUE key) { //{{{
  node_struct *ns;
  Data_Get_Struct(key, node_struct, ns);

  UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(ns->id);

  UA_MonitoredItemCreateResult monResponse =
  UA_Client_MonitoredItems_createDataChange(ns->client->client, ns->client->subscription_response.subscriptionId,
                                            UA_TIMESTAMPSTORETURN_BOTH,
                                            monRequest, (void *)ns->on_change, client_run_handler, NULL);

  if(monResponse.statusCode != UA_STATUSCODE_GOOD) {
    rb_raise(rb_eRuntimeError, "Monitoring item failed: %s\n", UA_StatusCode_name(monResponse.statusCode));
  }
} //}}}
static VALUE client_run(VALUE self) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (pss->firstrun) {
    pss->firstrun = false;
    pss->subs_changed = false;
    pss->subscription_response = UA_Client_Subscriptions_create(pss->client, pss->subscription_request, NULL, NULL, NULL);
    if (pss->subscription_response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
      rb_raise(rb_eRuntimeError, "Subscription could not be created.");

    for (int i = 0; i < RARRAY_LEN(pss->subs); i++) {
      client_run_iterate(rb_ary_entry(pss->subs,i));
    }
  }
  UA_Client_run_iterate(pss->client, 100);

  return self;
} //}}}
static VALUE client_disconnect(VALUE self) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);
  if (!pss->started) rb_raise(rb_eRuntimeError, "Client disconnected.");

  if (!pss->firstrun) {
    // do we need to delete the individual monResponse (#UA_MonitoredItemCreateResult_clear)?
    UA_Client_Subscriptions_deleteSingle(pss->client,pss->subscription_response.subscriptionId);
  }
  UA_Client_disconnect(pss->client);
  pss->started = false;

  return self;
} //}}}

void Init_client(void) {
  mOPCUA = rb_define_module("OPCUA");

  cClient = rb_define_class_under(mOPCUA, "Client", rb_cObject);
  cNode   = rb_define_class_under(mOPCUA, "cNode", rb_cObject);

  rb_define_alloc_func(cClient, client_alloc);
  rb_define_method(cClient, "initialize", client_init, 3);
  rb_define_method(cClient, "disconnect", client_disconnect, 0);
  rb_define_method(cClient, "get", client_get, 2);
  rb_define_method(cClient, "check_subscription", client_run, 0);
  rb_define_method(cClient, "subscription_interval", client_subscription_interval, 0);
  rb_define_method(cClient, "subscription_interval=", client_subscription_interval_set, 1);
  rb_define_method(cClient, "debug", client_debug, 0);
  rb_define_method(cClient, "debug=", client_debug_set, 1);

  rb_define_method(cNode, "value", node_value, 0);
  rb_define_method(cNode, "on_change", node_on_change, 0);
}
