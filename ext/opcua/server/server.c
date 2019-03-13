#include "server.h"

VALUE mOPCUA = Qnil;
VALUE cServer = Qnil;
VALUE cObjectsNode = Qnil;
VALUE cTypesTopNode = Qnil;
VALUE cTypesSubNode = Qnil;
VALUE cLeafNode = Qnil;

/* -- */
void  node_free(node_struct *ns) { //{{{
  if (ns != NULL) { free(ns); }
} //}}}
VALUE node_alloc(VALUE klass, server_struct *server, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->server = server;
  ns->id  = nodeid;

	return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */
VALUE node_add_object_type(VALUE self, VALUE name) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId n = UA_NODEID_STRING(ns->server->default_ns, nstr);

  UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
                          dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectTypeNode(ns->server->server,
                              n,
                              ns->id,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(ns->server->default_ns, nstr),
                              dtAttr,
                              NULL,
                              NULL);

  return node_alloc(cTypesSubNode,ns->server,n);
} //}}}
VALUE node_add_variable(VALUE self, VALUE name) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
  mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);

  UA_NodeId n = UA_NODEID_STRING(ns->server->default_ns, nstr);

  UA_Server_addVariableNode(ns->server->server,
                            n,
                            ns->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns->server->default_ns, nstr),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            mnAttr,
                            NULL,
                            NULL);

  return node_alloc(cLeafNode,ns->server,n);
} //}}}

/* -- */
void server_free(server_struct *pss) { //{{{
  if (pss != NULL) {
    UA_Server_delete(pss->server);
    UA_ServerConfig_delete(pss->config);
    free(pss);
  }
} //}}}
VALUE server_alloc(VALUE self) { //{{{
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->config = UA_ServerConfig_new_default();
  pss->server = UA_Server_new(pss->config);
  pss->default_ns = 1;

	return Data_Wrap_Struct(self, NULL, server_free, pss);
} //}}}
/* ++ */ //}}}
VALUE server_init(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->server);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
} //}}}
VALUE server_run(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->server, false);

  return rb_float_new(timeout/1000.0);
} //}}}
VALUE server_add_namespace(VALUE self, VALUE name) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  VALUE str;
  str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  pss->default_ns = UA_Server_addNamespace(pss->server, nstr);
  return self;
} //}}}
VALUE server_types(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_alloc(cTypesTopNode, pss, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE));
} //}}}
VALUE server_objects(VALUE self) { //{{{
  return self;
} //}}}

void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");

  rb_define_const(mOPCUA, "TYPES_DOUBLE", INT2NUM(UA_TYPES_DOUBLE));

  cServer = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cObjectsNode  = rb_define_class_under(mOPCUA, "cObjectsNode", rb_cObject);
  cTypesTopNode  = rb_define_class_under(mOPCUA, "cTypesTopNode", rb_cObject);
  cTypesSubNode  = rb_define_class_under(mOPCUA, "cTypesSubNode", rb_cObject);
  cLeafNode  = rb_define_class_under(mOPCUA, "cLeafNode", rb_cObject);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", (VALUE(*)(ANYARGS))server_run, 0);
  rb_define_method(cServer, "add_namespace", (VALUE(*)(ANYARGS))server_add_namespace, 1);
  rb_define_method(cServer, "types", (VALUE(*)(ANYARGS))server_types, 0);
  rb_define_method(cServer, "objects", (VALUE(*)(ANYARGS))server_objects, 0);

  rb_define_method(cTypesTopNode, "add_object_type", (VALUE(*)(ANYARGS))node_add_object_type, 1);
  rb_define_method(cTypesSubNode, "add_object_type", (VALUE(*)(ANYARGS))node_add_object_type, 1);
  rb_define_method(cTypesSubNode, "add_variable", (VALUE(*)(ANYARGS))node_add_variable, 1);

  // rb_define_alloc_func(cONode, conode_alloc);
  // rb_define_method(cONode, "initialize", conode_init, 0);
  // rb_define_method(cONode, "add_object", (VALUE(*)(ANYARGS))conode_add_object, 1);
  // rb_define_method(cONode, "add_variable", (VALUE(*)(ANYARGS))conode_add_variable, 1);
}
