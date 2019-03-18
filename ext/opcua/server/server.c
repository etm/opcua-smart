#include "server.h"

VALUE mOPCUA = Qnil;
VALUE cServer = Qnil;
VALUE cObjectsNode = Qnil;
VALUE cTypesTopNode = Qnil;
VALUE cTypesSubNode = Qnil;
VALUE cLeafNode = Qnil;

/* -- */
static void  node_free(node_struct *ns) { //{{{
  if (ns != NULL) { free(ns); }
} //}}}
static VALUE node_alloc(VALUE klass, server_struct *server, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->server = server;
  ns->id  = nodeid;

	return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */
static VALUE node_type_folder(VALUE self) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  return node_alloc(cTypesTopNode, ns->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
} //}}}
static VALUE node_add_object_type(VALUE self, VALUE name) { //{{{
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
static VALUE node_add_variable(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *ns;

  if (argc > 2 || argc == 0) {  // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 2 && argv[1] != Qnil) {
    type = NUM2INT(argv[1]);
  } else {
    type = UA_NS0ID_MODELLINGRULE_MANDATORY;
  }

  Data_Get_Struct(self, node_struct, ns);

  VALUE str = rb_obj_as_string(argv[0]);
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

  if (argv[1] != Qnil) {
    UA_Server_addReference(ns->server->server,
                           n,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, type),
                           true);
  }

  return node_alloc(cLeafNode,ns->server,n);
} //}}}
static VALUE node_add_variable_without(VALUE self, VALUE name) { //{{{
  VALUE argv[] = { name, Qnil };
  return node_add_variable(2,argv,self);
} //}}}
static VALUE node_add_object(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *ns;
  node_struct *ts;

  if (argc > 3 || argc < 2) {  // there should only be 2 or 3 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 3 && argv[2] != Qnil) {
    type = NUM2INT(argv[2]);
  } else {
    type = UA_NS0ID_MODELLINGRULE_MANDATORY;
  }

  if (!(rb_obj_is_kind_of(argv[1],cTypesTopNode) || rb_obj_is_kind_of(argv[1],cTypesSubNode))) {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, ns);
  Data_Get_Struct(argv[1], node_struct, ts);

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId n = UA_NODEID_STRING(ns->server->default_ns, nstr);

  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                      oAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectNode(ns->server->server,
                          n,
                          ns->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          UA_QUALIFIEDNAME(ns->server->default_ns, nstr),
                          ts->id,
                          oAttr,
                          NULL,
                          NULL);

  if (argv[2] != Qnil) {
    UA_Server_addReference(ns->server->server,
                           n,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, type),
                           true);
  }

  return node_alloc(CLASS_OF(self),ns->server,n);
} //}}}
static VALUE node_add_object_without(VALUE self, VALUE name, VALUE parent) { //{{{
  VALUE argv[] = { name, parent, Qnil };
  return node_add_object(3,argv,self);
} //}}}
static VALUE node_find(VALUE self, VALUE qname) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  VALUE str = rb_obj_as_string(qname);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  /* Find the NodeId of the status child variable */
  UA_RelativePathElement rpe;
  UA_RelativePathElement_init(&rpe);
  rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
  rpe.isInverse = false;
  rpe.includeSubtypes = false;
  rpe.targetName = UA_QUALIFIEDNAME(ns->server->default_ns, nstr);

  UA_BrowsePath bp;
  UA_BrowsePath_init(&bp);
  bp.startingNode = ns->id;
  bp.relativePath.elementsSize = 1;
  bp.relativePath.elements = &rpe;

  UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(ns->server->server, &bp);

  if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
    return Qnil;
  } else {
    UA_NodeId ret = bpr.targets[0].targetId.nodeId;
    UA_BrowsePathResult_clear(&bpr);
    return node_alloc(CLASS_OF(self),ns->server,ret);
  }
} //}}}

static VALUE node_set_value(VALUE self, VALUE value) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Variant variant;
  if (rb_obj_is_kind_of(value,rb_cTime)) {
    UA_DateTime tmp = UA_DateTime_fromUnixTime(rb_time_timeval(value).tv_sec);
    UA_Variant_setScalar(&variant, &tmp, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(ns->server->server, ns->id, variant);
  } else {
    switch (TYPE(value)) {
      case T_FALSE:
        {
          UA_Boolean tmp = false;
          UA_Variant_setScalar(&variant, &tmp, &UA_TYPES[UA_TYPES_BOOLEAN]);
          UA_Server_writeValue(ns->server->server, ns->id, variant);
          break;
        }
      case T_TRUE:
        {
          UA_Boolean tmp = true;
          UA_Variant_setScalar(&variant, &tmp, &UA_TYPES[UA_TYPES_BOOLEAN]);
          UA_Server_writeValue(ns->server->server, ns->id, variant);
          break;
        }
      case T_FLOAT:
      case T_FIXNUM:
        {
          UA_Double tmp = NUM2DBL(value);
          UA_Variant_setScalar(&variant, &tmp, &UA_TYPES[UA_TYPES_DOUBLE]);
          UA_Server_writeValue(ns->server->server, ns->id, variant);
          break;
        }
      case T_STRING:
      case T_SYMBOL:
        {
          VALUE str = rb_obj_as_string(value);
          if (NIL_P(str) || TYPE(str) != T_STRING)
            rb_raise(rb_eTypeError, "cannot convert obj to string");
          UA_String tmp = UA_STRING(StringValuePtr(str));
          UA_Variant_setScalar(&variant, &tmp, &UA_TYPES[UA_TYPES_STRING]);
          UA_Server_writeValue(ns->server->server, ns->id, variant);
          break;
        }
    }
  }
  return Qnil;
} //}}}
static VALUE node_value(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(ns->server->server, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
      UA_DateTime raw = *(UA_DateTime *) value.data;
      ret = rb_time_new(UA_DateTime_toUnixTime(raw),0);
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
      UA_Boolean raw = *(UA_Boolean *) value.data;
      ret = raw ? Qtrue : Qfalse;
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
      UA_Double raw = *(UA_Double *) value.data;
      ret = DBL2NUM(raw);
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
      UA_String raw = *(UA_String *) value.data;
      ret = rb_str_new((char *)(raw.data),raw.length);
    }
  }

  UA_Variant_clear(&value);
  return ret;
} //}}}
/* -- */
static void  server_free(server_struct *pss) { //{{{
  if (pss != NULL) {
    UA_Server_delete(pss->server);
    UA_ServerConfig_delete(pss->config);
    free(pss);
  }
} //}}}
static VALUE server_alloc(VALUE self) { //{{{
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
static VALUE server_init(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->server);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
} //}}}
static VALUE server_run(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->server, false);

  return rb_float_new(timeout/1000.0);
} //}}}
static VALUE server_add_namespace(VALUE self, VALUE name) { //{{{
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
static VALUE server_types(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_alloc(cTypesTopNode, pss, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE));
} //}}}
static VALUE server_objects(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_alloc(cObjectsNode, pss, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER));
} //}}}

void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");
  rb_define_const(mOPCUA, "MANDATORY", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORY));
  rb_define_const(mOPCUA, "MANDATORYPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORYPLACEHOLDER));
  rb_define_const(mOPCUA, "OPTIONAL", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONAL));
  rb_define_const(mOPCUA, "OPTIONALPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONALPLACEHOLDER));

  cServer = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cObjectsNode  = rb_define_class_under(mOPCUA, "cObjectsNode", rb_cObject);
  cTypesTopNode  = rb_define_class_under(mOPCUA, "cTypesTopNode", rb_cObject);
  cTypesSubNode  = rb_define_class_under(mOPCUA, "cTypesSubNode", rb_cObject);
  cLeafNode  = rb_define_class_under(mOPCUA, "cLeafNode", rb_cObject);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", server_run, 0);
  rb_define_method(cServer, "add_namespace", server_add_namespace, 1);
  rb_define_method(cServer, "types", server_types, 0);
  rb_define_method(cServer, "objects", server_objects, 0);

  rb_define_method(cTypesTopNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypesTopNode, "folder", node_type_folder, 0);
  rb_define_method(cTypesSubNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypesSubNode, "add_variable", node_add_variable, -1);
  rb_define_method(cTypesSubNode, "add_object", node_add_object, -1);

  rb_define_method(cObjectsNode, "instantiate", node_add_object_without, 2);
  rb_define_method(cObjectsNode, "add_variable", node_add_variable_without, 1);
  rb_define_method(cObjectsNode, "find", node_find, 1);
  rb_define_method(cObjectsNode, "value", node_value, 0);
  rb_define_method(cObjectsNode, "value=", node_set_value, 1);
}

/*
  Questions:
    UA_NS0ID_HASSUBTYPE bei add_object_type???
    UA_NS0ID_HASCOMPONENT bei add_variable???
    UA_NS0ID_ORGANIZES bei calimat???
*/
