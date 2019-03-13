#include "server.h"

VALUE mOPCUA = Qnil;
VALUE cServer = Qnil;
VALUE cNodeId = Qnil;

/* -- */
// ***********************************************************************************
// GC
// ***********************************************************************************
void server_free(server_struct *pss) {
  if (pss != NULL) {
    UA_Server_delete(pss->server);
    UA_ServerConfig_delete(pss->config);
    free(pss);
  }
}
/* ++ */

VALUE server_alloc(VALUE self) {
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->config = UA_ServerConfig_new_default();
  pss->server = UA_Server_new(pss->config);
  pss->nodes = rb_hash_new();

	return Data_Wrap_Struct(self, NULL, server_free, pss);
}
VALUE server_init(VALUE self) {
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->server);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
}

VALUE server_run(VALUE self) {
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->server, false);

  return rb_float_new(timeout/1000.0);
}

VALUE server_add_object_type(VALUE self, VALUE name) {
  server_struct *pss;
  nodeid_struct *pns;

  Data_Get_Struct(self, server_struct, pss);

  Check_Type(name, T_SYMBOL);

  VALUE str;
  str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  pns = (nodeid_struct *)malloc(sizeof(nodeid_struct));
  pns->name = name;
  pns->id = UA_NODEID_STRING(1, nstr);
  rb_hash_aset(pss->nodes, name, Data_Wrap_Struct(cNodeId, NULL, NULL, pns));

  UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;

  dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectTypeNode(pss->server,
                              UA_NODEID_NULL,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, nstr),
                              dtAttr,
                              NULL,
                              &(pns->id));

  return self;
}

VALUE server_add_variable(VALUE self, VALUE parent, VALUE name) {
  server_struct *pss;
  nodeid_struct *pns;
  nodeid_struct *par;

  Data_Get_Struct(self, server_struct, pss);

  Check_Type(parent, T_SYMBOL);
  Check_Type(name, T_SYMBOL);

  VALUE str;

  str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_VariableAttributes mnAttr = UA_VariableAttributes_default;

  pns = (nodeid_struct *)malloc(sizeof(nodeid_struct));
  pns->name = name;
  pns->id = UA_NODEID_STRING(1, nstr);
  rb_hash_aset(pss->nodes, name, Data_Wrap_Struct(cNodeId, NULL, NULL, pns));

  Data_Get_Struct(rb_hash_aref(pss->nodes, parent), nodeid_struct, par);

  UA_Server_addVariableNode(pss->server,
                            UA_NODEID_NULL,
                            par->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, nstr),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            mnAttr,
                            NULL,
                            &(pns->id));

  return self;
}



void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");

  rb_define_const(mOPCUA, "TYPES_DOUBLE", INT2NUM(UA_TYPES_DOUBLE));

  cServer = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cNodeId = rb_define_class_under(mOPCUA, "NodeId", rb_cObject);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", (VALUE(*)(ANYARGS))server_run, 0);
  rb_define_method(cServer, "add_object_type", (VALUE(*)(ANYARGS))server_add_object_type, 1);
  rb_define_method(cServer, "add_variable", (VALUE(*)(ANYARGS))server_add_variable, 2);
}
