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
static node_struct * node_alloc(server_struct *server, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->server = server;
  ns->id  = nodeid;

	return ns;
} //}}}
static VALUE node_wrap(VALUE klass, node_struct *ns) { //{{{
	return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */
static VALUE node_type_folder(VALUE self) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  return node_wrap(cTypesTopNode, node_alloc(ns->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE)));
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

  return node_wrap(cTypesSubNode,node_alloc(ns->server,n));
} //}}}
static UA_NodeId node_add_variable_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, VALUE ref) { //{{{
  UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
  mnAttr.displayName = dn;

  UA_Server_addVariableNode(parent->server->server,
                            n,
                            parent->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            qn,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            mnAttr,
                            NULL,
                            NULL);

  if (ref != Qnil) {
    UA_Server_addReference(parent->server->server,
                           n,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, type),
                           true);
  }

  return n;
} //}}}
static UA_NodeId node_add_variable_ua_simple(UA_Int32 type, char* nstr, node_struct *parent, VALUE ref) { //{{{
  return node_add_variable_ua(
    type,
    UA_NODEID_STRING(parent->server->default_ns,nstr),
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->server->default_ns, nstr),
    parent,
    ref
  );
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

  return node_wrap(cLeafNode,node_alloc(ns->server,node_add_variable_ua_simple(type,nstr,ns,argv[1])));
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

  return node_wrap(CLASS_OF(self),node_alloc(ns->server,n));
} //}}}
static VALUE node_add_object_without(VALUE self, VALUE name, VALUE parent) { //{{{
  VALUE argv[] = { name, parent, Qnil };
  return node_add_object(3,argv,self);
} //}}}
static UA_StatusCode node_manifest_iter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) { //{{{
    if (isInverse) return UA_STATUSCODE_GOOD;

    node_struct **tandle = (node_struct **)handle;
    node_struct *parent = tandle[0];
    node_struct *newnode = tandle[1];

    if (childId.namespaceIndex == parent->server->default_ns) {
      UA_NodeClass *nc = UA_NodeClass_new();
      UA_LocalizedText *dn = UA_LocalizedText_new();
      UA_QualifiedName *qn = UA_QualifiedName_new();
      UA_QualifiedName *tqn = UA_QualifiedName_new();
      UA_Server_readNodeClass(parent->server->server, childId, nc);
      UA_Server_readBrowseName(parent->server->server, childId, qn);
      UA_Server_readDisplayName(parent->server->server, childId, dn);
      UA_Server_readBrowseName(parent->server->server, parent->id, tqn);
      printf("%d ---> NodeId %d, %-16.*s, %-16.*s, ref: %d, nc: %d\n",
             referenceTypeId.identifier.numeric,
             childId.namespaceIndex,
             (int)tqn->name.length,
             tqn->name.data,
             (int)qn->name.length,
             qn->name.data,
             referenceTypeId.identifier.numeric,
             *nc
      );

      if (childId.namespaceIndex == parent->server->default_ns) {
        UA_QualifiedName mqn;
        UA_QualifiedName_init(&mqn);
        UA_Server_readBrowseName(parent->server->server, UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), &mqn);

        UA_RelativePathElement rpe;
        UA_RelativePathElement_init(&rpe);
        rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);
        rpe.isInverse = false;
        rpe.includeSubtypes = false;
        rpe.targetName = mqn;

        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.startingNode = childId;
        bp.relativePath.elementsSize = 1;
        bp.relativePath.elements = &rpe;

        UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(parent->server->server, &bp);

        if(bpr.statusCode == UA_STATUSCODE_GOOD && *nc == UA_NODECLASS_OBJECT) {
          node_struct *ns = node_alloc(parent->server,childId);
          node_struct *downhandle[2] = { ns, newnode };

          printf("---->\n");
          UA_Server_forEachChildNodeCall(parent->server->server, childId, node_manifest_iter, (void *)downhandle);
          printf("<----\n");

          free(ns);
        }
        if(bpr.statusCode == UA_STATUSCODE_GOOD && *nc == UA_NODECLASS_VARIABLE) {
          node_add_variable_ua(UA_NS0ID_MODELLINGRULE_MANDATORY,UA_NODEID_STRING(parent->server->default_ns,"blarg"),*dn,*qn,newnode,Qtrue);
        }
        UA_BrowsePathResult_clear(&bpr);
        UA_QualifiedName_clear(&mqn);
      }

      UA_NodeClass_delete(nc);
      UA_QualifiedName_delete(qn);
    }
    return UA_STATUSCODE_GOOD;
} //}}}
static VALUE node_manifest(VALUE self, VALUE name, VALUE parent) { //{{{
  node_struct *ns;
  node_struct *ts;

  if (!(rb_obj_is_kind_of(parent,cTypesTopNode) || rb_obj_is_kind_of(parent,cTypesSubNode))) {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, ns);
  Data_Get_Struct(parent, node_struct, ts);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId n = UA_NODEID_STRING(ns->server->default_ns, nstr);

  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                      oAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);

  UA_Server_addNode_begin(ns->server->server,
                          UA_NODECLASS_OBJECT,
                          n,
                          ns->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          UA_QUALIFIEDNAME(ns->server->default_ns, nstr),
                          ts->id,
                          (const UA_NodeAttributes*)&oAttr,
                          &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                          NULL,
                          NULL);

  node_struct *ret = node_alloc(ns->server,n);
  node_struct *handle[2] = { ts, ret };
  UA_Server_forEachChildNodeCall(ns->server->server, ts->id, node_manifest_iter, (void *)handle);

  UA_Server_addNode_finish(ns->server->server,n);

	return Data_Wrap_Struct(CLASS_OF(self), NULL, node_free, ret);
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
    return node_wrap(CLASS_OF(self),node_alloc(ns->server,ret));
  }
} //}}}

static VALUE node_value_set(VALUE self, VALUE value) { //{{{
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
      case T_ARRAY:
        {
          // UA_UInt32 arrayDims = 0;
          // attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
          // attr.arrayDimensions = &arrayDims;
          // attr.arrayDimensionsSize = 1;
          // UA_Variant_setArray(&attr.value, UA_Array_new(10, &UA_TYPES[type]), 10, &UA_TYPES[type]);
        }

    }
  }
  return self;
} //}}}
static VALUE node_value(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(ns->server->server, ns->id, &value);


  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  return ret;
} //}}}
static VALUE node_id(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  VALUE ret = rb_ary_new();

  rb_ary_push(ret,UINT2NUM(ns->id.namespaceIndex));
  if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC) {
    VALUE id = UINT2NUM((UA_UInt32)(ns->id.identifier.numeric));
    rb_ary_push(ret,id);
  } else if (ns->id.identifierType == UA_NODEIDTYPE_STRING) {
    rb_ary_push(ret,rb_str_new((const char *)ns->id.identifier.string.data,ns->id.identifier.string.length));
  } else if (ns->id.identifierType == UA_NODEIDTYPE_BYTESTRING) {
    rb_ary_push(ret,rb_str_new((const char *)ns->id.identifier.byteString.data,ns->id.identifier.byteString.length));
  }
  return ret;
} //}}}
/* -- */
static void  server_free(server_struct *pss) { //{{{
  if (pss != NULL) {
    UA_Server_delete(pss->server);
    free(pss);
  }
} //}}}
static VALUE server_alloc(VALUE self) { //{{{
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->server = UA_Server_new();
  pss->config = UA_Server_getConfig(pss->server);
  pss->default_ns = 1;

  UA_ServerConfig_setDefault(pss->config);

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
  return node_wrap(cTypesTopNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE)));
} //}}}
static VALUE server_objects(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cObjectsNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)));
} //}}}

void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");
  rb_define_const(mOPCUA, "MANDATORY", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORY));
  rb_define_const(mOPCUA, "MANDATORYPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORYPLACEHOLDER));
  rb_define_const(mOPCUA, "OPTIONAL", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONAL));
  rb_define_const(mOPCUA, "OPTIONALPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONALPLACEHOLDER));

  cServer       = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cObjectsNode  = rb_define_class_under(mOPCUA, "cObjectsNode", rb_cObject);
  cTypesTopNode = rb_define_class_under(mOPCUA, "cTypesTopNode", rb_cObject);
  cTypesSubNode = rb_define_class_under(mOPCUA, "cTypesSubNode", rb_cObject);
  cLeafNode     = rb_define_class_under(mOPCUA, "cLeafNode", rb_cObject);

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
  rb_define_method(cTypesSubNode, "id", node_id, 0);

  rb_define_method(cObjectsNode, "instantiate", node_add_object_without, 2);
  rb_define_method(cObjectsNode, "manifest", node_manifest, 2);
  rb_define_method(cObjectsNode, "add_variable", node_add_variable_without, 1);
  rb_define_method(cObjectsNode, "find", node_find, 1);
  rb_define_method(cObjectsNode, "value", node_value, 0);
  rb_define_method(cObjectsNode, "value=", node_value_set, 1);
  rb_define_method(cObjectsNode, "id", node_id, 0);
}
