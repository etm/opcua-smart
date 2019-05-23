#include "server.h"

VALUE mOPCUA = Qnil;
VALUE mTYPES = Qnil;
VALUE cServer = Qnil;
VALUE cObjectsNode = Qnil;
VALUE cTypesTopNode = Qnil;
VALUE cTypesSubNode = Qnil;
VALUE cLeafNode = Qnil;

#include "../values.h"

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

static VALUE node_to_s(VALUE self) { //{{{
  node_struct *ns;
  VALUE ret;

  Data_Get_Struct(self, node_struct, ns);

  if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC) {
    ret = rb_sprintf("ns=%d;n=%d", ns->id.namespaceIndex, ns->id.identifier.numeric);
  } else if(ns->id.identifierType == UA_NODEIDTYPE_STRING) {
    ret = rb_sprintf("ns=%d;s=%.*s", ns->id.namespaceIndex, (int)ns->id.identifier.string.length, ns->id.identifier.string.data);
  } else {
    ret = rb_sprintf("ns=%d;unsupported",ns->id.namespaceIndex);
  }
  return ret;
} //}}}
static UA_StatusCode node_add_method_callback(
  UA_Server *server,
  const UA_NodeId *sessionId, void *sessionContext,
  const UA_NodeId *methodId, void *methodContext,
  const UA_NodeId *objectId, void *objectContext,
  size_t inputSize, const UA_Variant *input,
  size_t outputSize, UA_Variant *output
) {
  return UA_STATUSCODE_GOOD;
}
static UA_NodeId node_add_method_ua(UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent,size_t inputArgumentsSize,const UA_Argument *inputArguments,VALUE blk) { //{{{
  UA_MethodAttributes mnAttr = UA_MethodAttributes_default;
  mnAttr.displayName = dn;
  mnAttr.executable = true;
  mnAttr.userExecutable = true;

  UA_Server_addMethodNode(parent->server->server,
                         n,
                         parent->id,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                         qn,
                         mnAttr,
                         &node_add_method_callback,
                         inputArgumentsSize,
                         inputArguments,
                         0,
                         NULL,
                         (void *)blk,
                         NULL);


  UA_Server_addReference(parent->server->server,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);

  return n;
} //}}}
static UA_NodeId node_add_method_ua_simple(char* nstr, node_struct *parent, VALUE opts, VALUE blk) { //{{{
  UA_Argument inputArguments[RHASH_SIZE(opts)];

  VALUE ary = rb_funcall(opts, rb_intern("to_a"), 0);
  for (long i=0; i<RARRAY_LEN(ary); i++) {
	  VALUE item = RARRAY_AREF(ary, i);
    VALUE str = rb_obj_as_string(RARRAY_AREF(item, 0));
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert obj to string");
    char *nstr = (char *)StringValuePtr(str);
    UA_Argument_init(&inputArguments[i]);
    inputArguments[i].description = UA_LOCALIZEDTEXT("en-US", nstr);
    inputArguments[i].name = UA_STRING(nstr);
    inputArguments[i].dataType = UA_TYPES[NUM2INT(RARRAY_AREF(item, 1))].typeId;
    inputArguments[i].valueRank = UA_VALUERANK_SCALAR;
  }

  return node_add_method_ua(
    UA_NODEID_STRING(parent->server->default_ns,nstr),
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->server->default_ns, nstr),
    parent,
    RHASH_SIZE(opts),
    inputArguments,
    blk
  );
} //}}}
static VALUE node_add_method(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *parent;

  VALUE name;
	VALUE opts;
	VALUE blk;

  if (argc < 2) {  // there should only be 2 or 3 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }
	rb_scan_args(argc, argv, "1:&", &name, &opts, &blk);
  if (NIL_P(opts)) opts = rb_hash_new();

  Data_Get_Struct(self, node_struct, parent);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self),node_alloc(parent->server,node_add_method_ua_simple(nstr,parent,opts,blk)));
} //}}}

static UA_NodeId node_add_variable_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, VALUE ref, UA_Byte accesslevelmask) { //{{{
  UA_VariableAttributes vAttr = UA_VariableAttributes_default;
  vAttr.displayName = dn;

  vAttr.accessLevel = accesslevelmask;

  UA_Server_addVariableNode(parent->server->server,
                            n,
                            parent->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            qn,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            vAttr,
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
static UA_NodeId node_add_variable_ua_simple(UA_Int32 type, char* nstr, node_struct *parent, VALUE ref, UA_Byte accesslevelmask) { //{{{
  return node_add_variable_ua(
    type,
    UA_NODEID_STRING(parent->server->default_ns,nstr),
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->server->default_ns, nstr),
    parent,
    ref,
    accesslevelmask
  );
} //}}}
static VALUE node_add_variable_wrap(int argc, VALUE* argv, VALUE self, UA_Byte accesslevelmask) { //{{{
  node_struct *parent;

  if (argc > 2 || argc == 0) {  // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 2 && argv[1] != Qnil) {
    type = NUM2INT(argv[1]);
  } else {
    type = UA_NS0ID_MODELLINGRULE_MANDATORY;
  }

  Data_Get_Struct(self, node_struct, parent);

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(cLeafNode,node_alloc(parent->server,node_add_variable_ua_simple(type,nstr,parent,argv[1],accesslevelmask)));
} //}}}
static VALUE node_add_variable(int argc, VALUE* argv, VALUE self) { //{{{
  return node_add_variable_wrap(argc,argv,self,UA_ACCESSLEVELMASK_READ);
} //}}}
static VALUE node_add_variable_rw(int argc, VALUE* argv, VALUE self) { //{{{
  return node_add_variable_wrap(argc,argv,self,UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
} //}}}
static VALUE node_add_variable_without(VALUE self, VALUE name) { //{{{
  VALUE argv[] = { name, Qnil };
  return node_add_variable(2,argv,self);
} //}}}
static VALUE node_add_variable_rw_without(VALUE self, VALUE name) { //{{{
  VALUE argv[] = { name, Qnil };
  return node_add_variable_rw(2,argv,self);
} //}}}

static UA_NodeId node_add_object_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, node_struct *datatype, VALUE ref) { //{{{
  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                      oAttr.displayName = dn;

  UA_Server_addObjectNode(parent->server->server,
                          n,
                          parent->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          qn,
                          datatype->id,
                          oAttr,
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
static UA_NodeId node_add_object_ua_simple(UA_Int32 type, char* nstr, node_struct *parent, node_struct *datatype, VALUE ref) { //{{{
  return node_add_object_ua(
    type,
    UA_NODEID_STRING(parent->server->default_ns,nstr),
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->server->default_ns, nstr),
    parent,
    datatype,
    ref
  );
}  //}}}
static VALUE node_add_object(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *parent;
  node_struct *datatype;

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

  Data_Get_Struct(self, node_struct, parent);
  Data_Get_Struct(argv[1], node_struct, datatype);

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self),node_alloc(parent->server,node_add_object_ua_simple(type,nstr,parent,datatype,argv[2])));
} //}}}
static VALUE node_add_object_without(VALUE self, VALUE name, VALUE parent) { //{{{
  VALUE argv[] = { name, parent, Qnil };
  return node_add_object(3,argv,self);
} //}}}

static UA_BrowsePathResult node_browse_path(UA_Server *server, UA_NodeId relative, UA_NodeId ref, UA_QualifiedName mqn) { //{{{
  UA_RelativePathElement rpe;
  UA_RelativePathElement_init(&rpe);
  rpe.referenceTypeId = ref;
  rpe.isInverse = false;
  rpe.includeSubtypes = false;
  rpe.targetName = mqn;

  UA_BrowsePath bp;
  UA_BrowsePath_init(&bp);
  bp.startingNode = relative;
  bp.relativePath.elementsSize = 1;
  bp.relativePath.elements = &rpe;

  return UA_Server_translateBrowsePathToNodeIds(server, &bp);
} //}}}

static bool node_get_reference(UA_Server *server, UA_NodeId parent, UA_NodeId *result) { //{{{
	UA_BrowseDescription bDes;
  UA_BrowseDescription_init(&bDes);
  bDes.nodeId = parent;
  bDes.resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResult bRes = UA_Server_browse(server, 1, &bDes);

  if (bRes.referencesSize > 0) {
    UA_ReferenceDescription *ref = &(bRes.references[0]);

    *result = ref->nodeId.nodeId;
    UA_BrowseResult_clear(&bRes);
    return true;
  }
  return false;
} //}}}

static UA_StatusCode node_manifest_iter(UA_NodeId child_id, UA_Boolean is_inverse, UA_NodeId reference_type_id, void *handle) { //{{{
    if (is_inverse) return UA_STATUSCODE_GOOD;

    node_struct **tandle = (node_struct **)handle;
    node_struct *parent = tandle[0];
    node_struct *newnode = tandle[1];

    if (child_id.namespaceIndex == parent->server->default_ns) {
      UA_NodeClass nc; UA_NodeClass_init(&nc);

      UA_LocalizedText dn;  UA_LocalizedText_init(&dn);
      UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
      UA_QualifiedName pqn; UA_QualifiedName_init(&pqn);
      UA_QualifiedName nqn; UA_QualifiedName_init(&nqn);

      UA_Byte al; UA_Byte_init(&al);

      UA_Server_readNodeClass(parent->server->server, child_id, &nc);
      UA_Server_readBrowseName(parent->server->server, child_id, &qn);
      UA_Server_readDisplayName(parent->server->server, child_id, &dn);
      UA_Server_readAccessLevel(parent->server->server, child_id, &al);
      UA_Server_readBrowseName(parent->server->server, parent->id, &pqn);
      UA_Server_readBrowseName(parent->server->server, newnode->id, &nqn);

      // printf("%d ---> NodeId %d, %-16.*s, %-16.*s, ref: %d, nc: %d\n",
      //        reference_type_id.identifier.numeric,
      //        child_id.namespaceIndex,
      //        (int)pqn.name.length,
      //        pqn.name.data,
      //        (int)qn.name.length,
      //        qn.name.data,
      //        reference_type_id.identifier.numeric,
      //        nc
      // );

      if (child_id.namespaceIndex == parent->server->default_ns) {
        UA_QualifiedName mqn;UA_QualifiedName_init(&mqn);
        UA_Server_readBrowseName(parent->server->server, UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), &mqn);

        UA_BrowsePathResult mandatory = node_browse_path(parent->server->server, child_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE), mqn);

        if (mandatory.statusCode == UA_STATUSCODE_GOOD && (nc == UA_NODECLASS_OBJECT || nc == UA_NODECLASS_VARIABLE)) {
          char * buffer = strnautocat(NULL,"",0);
          if (newnode->id.identifier.string.data[0] != '/') {
            buffer = strnautocat(buffer,"/",1);
          }
          buffer = strnautocat(buffer,(char *)newnode->id.identifier.string.data,newnode->id.identifier.string.length);
          buffer = strnautocat(buffer,"/",1);
          buffer = strnautocat(buffer,(char *)qn.name.data,qn.name.length);
          if(nc == UA_NODECLASS_OBJECT) {
            UA_NodeId typeid;
            node_get_reference(parent->server->server, child_id, &typeid);

            node_struct *thetype = node_alloc(parent->server,typeid);
            node_struct *downnode = node_alloc(parent->server,node_add_object_ua(UA_NS0ID_MODELLINGRULE_MANDATORY,UA_NODEID_STRING(parent->server->default_ns,buffer),dn,qn,newnode,thetype,Qtrue));

            node_struct *newparent = node_alloc(parent->server,child_id);
            node_struct *downhandle[2] = { newparent, downnode };

            // printf("---->\n");
            UA_Server_forEachChildNodeCall(parent->server->server, child_id, node_manifest_iter, (void *)downhandle);
            // printf("<----\n");

            free(thetype);
            free(downnode);
            free(newparent);
          }
          if(nc == UA_NODECLASS_VARIABLE) {
            node_add_variable_ua(UA_NS0ID_MODELLINGRULE_MANDATORY,UA_NODEID_STRING(parent->server->default_ns,buffer),dn,qn,newnode,Qtrue,al);
          }
        }
        UA_BrowsePathResult_clear(&mandatory);
        UA_QualifiedName_clear(&mqn);
      }

      UA_NodeClass_clear(&nc);
      UA_Byte_clear(&al);
      UA_QualifiedName_clear(&qn);
      UA_QualifiedName_clear(&pqn);
      UA_LocalizedText_clear(&dn);
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

  char *nidstr = strnautocat(NULL,"",1);
  if (ns->id.identifierType == UA_NODEIDTYPE_STRING) {
    nidstr = strnautocat(nidstr,(char *)ns->id.identifier.string.data,ns->id.identifier.string.length);
    nidstr = strnautocat(nidstr,"/",1);
  }
  nidstr = strnautocat(nidstr,nstr,strlen(nstr));

  UA_NodeId n = UA_NODEID_STRING(ns->server->default_ns, nidstr);

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

  UA_BrowsePathResult bpr = node_browse_path(ns->server->server, ns->id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(ns->server->default_ns, nstr));

  if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
    return Qnil;
  } else {
    UA_NodeId ret;UA_NodeId_init(&ret);
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId,&ret);
    UA_BrowsePathResult_clear(&bpr);
    return node_wrap(CLASS_OF(self),node_alloc(ns->server,ret));
  }
} //}}}

static VALUE node_value_set(VALUE self, VALUE value) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  set_node_to_value(ns,value);
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
/* ++ */

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

  Init_types();

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
  rb_define_method(cTypesSubNode, "add_variable_rw", node_add_variable_rw, -1);
  rb_define_method(cTypesSubNode, "add_object", node_add_object, -1);
  rb_define_method(cTypesSubNode, "add_method", node_add_method, -1);
  rb_define_method(cTypesSubNode, "id", node_id, 0);

  rb_define_method(cObjectsNode, "instantiate", node_add_object_without, 2);
  rb_define_method(cObjectsNode, "manifest", node_manifest, 2);
  rb_define_method(cObjectsNode, "add_variable", node_add_variable_without, 1);
  rb_define_method(cObjectsNode, "add_variable_rw", node_add_variable_rw_without, 1);
  rb_define_method(cObjectsNode, "find", node_find, 1);
  rb_define_method(cObjectsNode, "value", node_value, 0);
  rb_define_method(cObjectsNode, "value=", node_value_set, 1);
  rb_define_method(cObjectsNode, "to_s", node_to_s, 0);
  rb_define_method(cObjectsNode, "id", node_id, 0);
}
