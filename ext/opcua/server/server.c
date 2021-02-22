#include "server.h"

VALUE mOPCUA = Qnil;
VALUE cServer = Qnil;
VALUE cNode = Qnil;
VALUE cObjectNode = Qnil;
VALUE cTypeTopNode = Qnil;
VALUE cTypeSubNode = Qnil;
VALUE cReferenceTopNode = Qnil;
VALUE cReferenceSubNode = Qnil;
VALUE cReferenceNode = Qnil;
VALUE cVarNode = Qnil;
VALUE cMethodNode = Qnil;

#include "values.h"

int nodecounter = 2000;

/* -- */
static void  node_mark(node_struct *ns) { //{{{
  if (ns == NULL) return;
  // if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC) {
  //   printf("mark ns=%d;i=%d\n", ns->id.namespaceIndex, ns->id.identifier.numeric);
  // } else if(ns->id.identifierType == UA_NODEIDTYPE_STRING) {
  //   printf("mark ns=%d;s=%.*s\n", ns->id.namespaceIndex, (int)ns->id.identifier.string.length, ns->id.identifier.string.data);
  // } else {
  //   printf("mark ns=%d;unsupported\n",ns->id.namespaceIndex);
  // }
  if (!NIL_P(ns->method)) rb_gc_mark(ns->method);
} // }}}
static void  node_free(node_struct *ns) { //{{{
  if (ns != NULL) {
    // if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");
    // if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC) {
    //   printf("delete ns=%d;i=%d\n", ns->id.namespaceIndex, ns->id.identifier.numeric);
    // } else if(ns->id.identifierType == UA_NODEIDTYPE_STRING) {
    //   printf("delete ns=%d;s=%.*s\n", ns->id.namespaceIndex, (int)ns->id.identifier.string.length, ns->id.identifier.string.data);
    // } else {
    //   printf("delete ns=%d;unsupported\n",ns->id.namespaceIndex);
    // }
    if (!NIL_P(ns->method)) {
      rb_gc_unregister_address(&ns->method);
    }
    free(ns);
  }
} // }}}
static node_struct * node_alloc(server_struct *server, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->master = server;
  ns->id     = nodeid;
  ns->method = Qnil;
  ns->exists = true;

	return ns;
} //}}}
static VALUE node_wrap(VALUE klass, node_struct *ns) { //{{{
	return Data_Wrap_Struct(klass, node_mark, node_free, ns);
} //}}}
/* ++ */

static VALUE node_type_folder(VALUE self) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");
  return node_wrap(cTypeTopNode, node_alloc(ns->master, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE)));
} //}}}
static VALUE node_add_object_type(VALUE self, VALUE name) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId n = UA_NODEID_NUMERIC(ns->master->default_ns, nodecounter++);

  UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
                          dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectTypeNode(ns->master->master,
                              n,
                              ns->id,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(ns->master->default_ns, nstr),
                              dtAttr,
                              NULL,
                              NULL);

  return node_wrap(cTypeSubNode,node_alloc(ns->master,n));
} //}}}
static VALUE node_add_reference_type(VALUE self, VALUE name, VALUE type) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert arg 1 to string");
  char *nstr = (char *)StringValuePtr(str);
  if (TYPE(type) != T_FIXNUM)
    rb_raise(rb_eTypeError, "cannot convert arg 2 to integer");

  UA_NodeId n = UA_NODEID_NUMERIC(ns->master->default_ns, nodecounter++);

  UA_ReferenceTypeAttributes rtAttr = UA_ReferenceTypeAttributes_default;
                             rtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addReferenceTypeNode(ns->master->master,
                                 n,
                                 ns->id,
                                 UA_NODEID_NUMERIC(0, NUM2INT(type)),
                                 UA_QUALIFIEDNAME(ns->master->default_ns, nstr),
                                 rtAttr,
                                 NULL,
                                 NULL);

  return node_wrap(cReferenceSubNode,node_alloc(ns->master,n));
} //}}}

static VALUE node_id(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

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
static VALUE node_name(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
  UA_Server_readBrowseName(ns->master->master, ns->id, &qn);

  return rb_sprintf("%.*s", (int)qn.name.length, qn.name.data);
} //}}}
static VALUE node_to_s(VALUE self) { //{{{
  node_struct *ns;
  VALUE ret;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC) {
    ret = rb_sprintf("ns=%d;i=%d", ns->id.namespaceIndex, ns->id.identifier.numeric);
  } else if(ns->id.identifierType == UA_NODEIDTYPE_STRING) {
    ret = rb_sprintf("ns=%d;s=%.*s", ns->id.namespaceIndex, (int)ns->id.identifier.string.length, ns->id.identifier.string.data);
  } else {
    ret = rb_sprintf("ns=%d;unsupported",ns->id.namespaceIndex);
  }
  return ret;
} //}}}
static VALUE node_add_reference(VALUE self, VALUE to, VALUE type) { //{{{
  node_struct *ns;
  node_struct *tos;
  node_struct *tys;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  if (!(rb_obj_is_kind_of(type,cReferenceSubNode) || rb_obj_is_kind_of(to,cTypeSubNode))) {
    rb_raise(rb_eArgError, "arguments have to be NodeIDs.");
  }
  Data_Get_Struct(to, node_struct, tos);
  if (!tos->exists) rb_raise(rb_eRuntimeError, "To (arg 1) node does not exist anymore.");
  Data_Get_Struct(type, node_struct, tys);
  if (!tys->exists) rb_raise(rb_eRuntimeError, "Type (arg 2) node does not exist anymore.");
  UA_NodeId n = UA_NODEID_NUMERIC(ns->master->default_ns, nodecounter++);

  UA_ExpandedNodeId toNodeId;
                    toNodeId.serverIndex = 0;
                    toNodeId.namespaceUri = UA_STRING_NULL;
                    toNodeId.nodeId = tos->id;

  UA_Server_addReference(ns->master->master,
                         n,
                         tys->id,
                         toNodeId,
                         true);

  return node_wrap(cReferenceNode,node_alloc(ns->master,n));
} //}}}

static UA_StatusCode node_add_method_callback( //{{{
  UA_Server *server,
  const UA_NodeId *sessionId, void *sessionContext,
  const UA_NodeId *methodId, void *methodContext,
  const UA_NodeId *objectId, void *objectContext,
  size_t inputSize, const UA_Variant *input,
  size_t outputSize, UA_Variant *output
) {
  node_struct *me = (node_struct *)methodContext;

  // printf(
  //   "NodeId %d, %-16.*s\n",
  //   me->id.namespaceIndex,
  //   (int)me->id.identifier.string.length,
  //   me->id.identifier.string.data
  // );

  VALUE args = rb_ary_new();
  rb_ary_push(args, Data_Wrap_Struct(cObjectNode,NULL,NULL,me));
  for (int i = 0; i < inputSize; i++) {
    VALUE ret = extract_value(input[i]);
    rb_ary_push(args,rb_ary_entry(ret,0));
  }

  VALUE ret = rb_proc_call(me->method,args);

  if (outputSize == 1) {
    value_to_variant(ret,output,-1);
  }

  return UA_STATUSCODE_GOOD;
} //}}}
static UA_NodeId node_add_method_ua(UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent,size_t inputArgumentsSize,const UA_Argument *inputArguments,size_t outputArgumentsSize,const UA_Argument *outputArguments, VALUE blk) { //{{{
  UA_MethodAttributes mnAttr = UA_MethodAttributes_default;
  mnAttr.displayName = dn;
  mnAttr.executable = true;
  mnAttr.userExecutable = true;

  node_struct *me = node_alloc(parent->master,n);
  me->method = blk;
  rb_gc_register_address(&blk);
  rb_gc_register_address(&me->method);

  UA_Server_addMethodNode(parent->master->master,
                         n,
                         parent->id,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                         qn,
                         mnAttr,
                         &node_add_method_callback,
                         inputArgumentsSize,
                         inputArguments,
                         outputArgumentsSize,
                         outputArguments,
                         (void *)me,
                         NULL);


  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);

  return n;
} //}}}
static UA_NodeId node_add_method_ua_simple(char* nstr, node_struct *parent, VALUE opts, VALUE blk) { //{{{
  UA_Argument inputArguments[RHASH_SIZE(opts)];
  UA_Argument outputArguments[1];
  int counter = 0;

  VALUE ary = rb_funcall(opts, rb_intern("to_a"), 0);
  for (long i=0; i<RARRAY_LEN(ary); i++) {
	  VALUE item = RARRAY_AREF(ary, i);
    VALUE str = rb_obj_as_string(RARRAY_AREF(item, 0));
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert obj to string");
    char *nstr = (char *)StringValuePtr(str);
    if (rb_str_cmp(str,rb_str_new2("return"))) {
      UA_Argument_init(&inputArguments[counter]);
      inputArguments[counter].description = UA_LOCALIZEDTEXT("en-US", nstr);
      inputArguments[counter].name = UA_STRING(nstr);
      inputArguments[counter].dataType = UA_TYPES[NUM2INT(RARRAY_AREF(item, 1))].typeId;
      inputArguments[counter].valueRank = UA_VALUERANK_SCALAR;
      counter++;
    } else {
      UA_Argument_init(&outputArguments[0]);
      outputArguments[0].description = UA_LOCALIZEDTEXT("en-US", nstr);
      outputArguments[0].name = UA_STRING(nstr);
      outputArguments[0].dataType = UA_TYPES[NUM2INT(RARRAY_AREF(item, 1))].typeId;
      outputArguments[0].valueRank = UA_VALUERANK_SCALAR;
    }
  }
  int nodeid = nodecounter++;

  rb_hash_aset(parent->master->methods,INT2NUM(nodeid),blk);
  rb_gc_register_address(&blk);

  return node_add_method_ua(
    UA_NODEID_NUMERIC(parent->master->default_ns,nodeid),
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
    parent,
    counter,
    inputArguments,
    RHASH_SIZE(opts)-counter,
    outputArguments,
    blk
  );
} //}}}
static VALUE node_add_method(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *parent;

  VALUE name;
	VALUE opts;
	VALUE blk;
  rb_gc_register_address(&blk);

  if (argc < 1) { // there should be 1 or more arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }
	rb_scan_args(argc, argv, "1:&", &name, &opts, &blk);
  if (NIL_P(opts)) opts = rb_hash_new();

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self),node_alloc(parent->master,node_add_method_ua_simple(nstr,parent,opts,blk)));
} //}}}

static void node_add_variable_callback( //{{{
  UA_Server *server,
  const UA_NodeId *sessionId, void *sessionContext,
  const UA_NodeId *nodeId, void *nodeContext,
  const UA_NumericRange *range, const UA_DataValue *data
) {
  node_struct *me = (node_struct *)nodeContext;
  UA_Guid internal = UA_GUID("00000001-0000-0000-0000-000000000000");

  // if (sessionId->identifierType == UA_NODEIDTYPE_NUMERIC) {
  //   printf("sessionId ns=%d;i=%d\n", sessionId->namespaceIndex, sessionId->identifier.numeric);
  // } else if(sessionId->identifierType == UA_NODEIDTYPE_STRING) {
  //   printf("sessionId ns=%d;s=%.*s\n", sessionId->namespaceIndex, (int)sessionId->identifier.string.length, sessionId->identifier.string.data);
  // } else if(sessionId->identifierType == UA_NODEIDTYPE_GUID) {
  //   if (UA_Guid_equal(&(sessionId->identifier.guid),&internal)) {
  //     printf("sessionId internal session\n");
  //   } else {
  //     printf("sessionId ns=%d;g=%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",sessionId->namespaceIndex,UA_PRINTF_GUID_DATA(sessionId->identifier.guid));
  //   }
  // } else if(sessionId->identifierType == UA_NODEIDTYPE_BYTESTRING) {
  //   printf("sessionId ns=%d;b=%.*s\n", sessionId->namespaceIndex, (int)sessionId->identifier.string.length, sessionId->identifier.string.data);
  // } else if(UA_NodeId_isNull(sessionId)) {
  //   printf("sessionId is null");
  // } else {
  //   printf("sessionId ns=%d;unsupported\n",sessionId->namespaceIndex);
  // }

  VALUE args = rb_ary_new();
  rb_ary_push(args, Data_Wrap_Struct(cVarNode,NULL,NULL,me));
  VALUE para = extract_value(data->value);
  rb_ary_push(args,rb_ary_entry(para,0));
  if (UA_Guid_equal(&(sessionId->identifier.guid),&internal)) {
    rb_ary_push(args,Qfalse);
  } else {
    rb_ary_push(args,Qtrue);
  }
  if (me->method != Qnil) {
    rb_proc_call(me->method,args);
  }
} //}}}
static UA_NodeId node_add_variable_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, UA_Byte accesslevelmask, VALUE blk) { //{{{
  UA_VariableAttributes vAttr = UA_VariableAttributes_default;
  vAttr.displayName = dn;
  vAttr.accessLevel = accesslevelmask;

  node_struct *me = node_alloc(parent->master,n);
  me->method = blk;
  rb_gc_register_address(&blk);
  rb_gc_register_address(&me->method);

  UA_Server_addVariableNode(parent->master->master,
                            n,
                            parent->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            qn,
                            UA_NODEID_NUMERIC(0, type),
                            vAttr,
                            (void *)me,
                            NULL);

  UA_ValueCallback callback ;
  callback.onWrite = node_add_variable_callback;
  UA_Server_setVariableNode_valueCallback(parent->master->master, n, callback);

  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);

  return n;
}  //}}}
static UA_NodeId node_add_variable_ua_simple(UA_Int32 type, VALUE str, node_struct *parent, UA_Byte accesslevelmask, bool numeric, VALUE blk) { //{{{
  int nodeid = nodecounter++;
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId n;
  if (numeric) {
    n =  UA_NODEID_NUMERIC(parent->master->default_ns,nodeid);
    rb_hash_aset(parent->master->methods,INT2NUM(nodeid),blk);
  } else {
    n = UA_NODEID_STRING(parent->master->default_ns,nstr);
    rb_hash_aset(parent->master->methods,str,blk);
  }
  rb_gc_register_address(&blk);
  return node_add_variable_ua(
    type,
    n,
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
    parent,
    accesslevelmask,
    blk
  );
} //}}}
static VALUE node_add_variable_wrap(int argc, VALUE* argv, VALUE self, UA_Byte accesslevelmask, bool numeric) { //{{{
  node_struct *parent;

  if (argc > 2 || argc == 0) { // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE name;
	VALUE ttype;
	VALUE blk;
  rb_gc_register_address(&blk);

	rb_scan_args(argc, argv, "11&", &name, &ttype, &blk);

  UA_UInt32 type;
  if (NIL_P(ttype)) {
    type = UA_NS0ID_BASEDATAVARIABLETYPE;
  } else {
    type = NUM2INT(ttype);
  }

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");

  return node_wrap(cVarNode,node_alloc(parent->master,node_add_variable_ua_simple(type,str,parent,accesslevelmask,numeric,blk)));
} //}}}
static VALUE node_add_variable(int argc, VALUE* argv, VALUE self) { //{{{
  return node_add_variable_wrap(argc,argv,self,UA_ACCESSLEVELMASK_READ,true);
} //}}}
static VALUE node_add_variable_rw(int argc, VALUE* argv, VALUE self) { //{{{
  return node_add_variable_wrap(argc,argv,self,UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE,true);
} //}}}

static void node_add_object_ua_rec(UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, node_struct *datatype, UA_NodeId cid, UA_NodeIteratorCallback callback, void *handle) { //{{{
  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                      oAttr.displayName = dn;
  UA_Server_addNode_begin(parent->master->master,
                          UA_NODECLASS_OBJECT,
                          n,
                          parent->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          qn,
                          datatype->id,
                          (const UA_NodeAttributes*)&oAttr,
                          &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                          NULL,
                          NULL);

  // printf("---->\n");
  UA_Server_forEachChildNodeCall(parent->master->master, cid, callback, handle);
  // printf("<----\n");

  UA_Server_addNode_finish(parent->master->master,n);
  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);
} //}}}
static UA_NodeId node_add_object_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, node_struct *datatype, VALUE ref) { //{{{
  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
                      oAttr.displayName = dn;

  UA_Server_addObjectNode(parent->master->master,
                          n,
                          parent->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          qn,
                          datatype->id,
                          oAttr,
                          NULL,
                          NULL);

  if (ref != Qnil) {
    UA_Server_addReference(parent->master->master,
                           n,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, type),
                           true);
  }

  return n;
} //}}}
static UA_NodeId node_add_object_ua_simple(UA_Int32 type, char* nstr, node_struct *parent, node_struct *datatype, VALUE ref, bool numeric) { //{{{
  int nodeid = nodecounter++;
  UA_NodeId n;
  if (numeric) {
    n =  UA_NODEID_NUMERIC(parent->master->default_ns,nodeid);
  } else {
    n = UA_NODEID_STRING(parent->master->default_ns,nstr);
  }

  return node_add_object_ua(
    type,
    n,
    UA_LOCALIZEDTEXT("en-US", nstr),
    UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
    parent,
    datatype,
    ref
  );
} //}}}
static VALUE node_add_object(int argc, VALUE* argv, VALUE self) { //{{{
  node_struct *parent;
  node_struct *datatype;

  if (argc > 3 || argc < 2) { // there should only be 2 or 3 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 3 && argv[2] != Qnil) {
    type = NUM2INT(argv[2]);
  } else {
    type = UA_NS0ID_MODELLINGRULE_MANDATORY;
  }

  if (!(rb_obj_is_kind_of(argv[1],cTypeTopNode) || rb_obj_is_kind_of(argv[1],cTypeSubNode))) {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists) rb_raise(rb_eRuntimeError, "Parent node does not exist anymore.");
  Data_Get_Struct(argv[1], node_struct, datatype);
  if (!datatype->exists) rb_raise(rb_eRuntimeError, "Datatype node does not exist anymore.");

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self),node_alloc(parent->master,node_add_object_ua_simple(type,nstr,parent,datatype,argv[2],true)));
} //}}}

static UA_StatusCode node_manifest_iter(UA_NodeId child_id, UA_Boolean is_inverse, UA_NodeId reference_type_id, void *handle) { //{{{
    if (is_inverse) return UA_STATUSCODE_GOOD;

    node_struct **tandle = (node_struct **)handle;
    node_struct *parent = tandle[0];
    node_struct *newnode = tandle[1];

    if (child_id.namespaceIndex == parent->master->default_ns) {
      UA_NodeClass nc; UA_NodeClass_init(&nc);

      UA_LocalizedText dn;  UA_LocalizedText_init(&dn);
      UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
      UA_QualifiedName pqn; UA_QualifiedName_init(&pqn);
      UA_QualifiedName nqn; UA_QualifiedName_init(&nqn);

      UA_Byte al; UA_Byte_init(&al);

      UA_Server_readNodeClass(parent->master->master, child_id, &nc);
      UA_Server_readBrowseName(parent->master->master, child_id, &qn);
      UA_Server_readDisplayName(parent->master->master, child_id, &dn);
      UA_Server_readAccessLevel(parent->master->master, child_id, &al);
      UA_Server_readBrowseName(parent->master->master, parent->id, &pqn);
      UA_Server_readBrowseName(parent->master->master, newnode->id, &nqn);

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

      if (child_id.namespaceIndex == parent->master->default_ns) {
        UA_QualifiedName mqn;UA_QualifiedName_init(&mqn);
        UA_Server_readBrowseName(parent->master->master, UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), &mqn);
        UA_BrowsePathResult mandatory = node_browse_path(parent->master->master, child_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE), mqn, false);
        UA_QualifiedName_clear(&mqn);

        if (mandatory.statusCode == UA_STATUSCODE_GOOD && (nc == UA_NODECLASS_OBJECT || nc == UA_NODECLASS_VARIABLE || nc == UA_NODECLASS_METHOD)) {
          char * buffer = strnautocat(NULL,"",0);
          if (newnode->id.identifier.string.data[0] != '/') {
            buffer = strnautocat(buffer,"/",1);
          }
          buffer = strnautocat(buffer,(char *)newnode->id.identifier.string.data,newnode->id.identifier.string.length);
          buffer = strnautocat(buffer,"/",1);
          buffer = strnautocat(buffer,(char *)qn.name.data,qn.name.length);
          if(nc == UA_NODECLASS_OBJECT) {
            UA_NodeId typeid;
            server_node_get_reference(parent->master->master, child_id, &typeid, false);

            node_struct *thetype = node_alloc(parent->master,typeid);

            UA_NodeId n = UA_NODEID_STRING(parent->master->default_ns,buffer);
            node_struct *downnode = node_alloc(parent->master,n);
            node_struct *newparent = node_alloc(parent->master,child_id);
            node_struct *downhandle[2] = { newparent, downnode };

            node_add_object_ua_rec(UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,thetype,child_id,node_manifest_iter,(void *)downhandle);

            free(thetype);
            free(downnode);
            free(newparent);
          }
          if(nc == UA_NODECLASS_VARIABLE) {
            UA_QualifiedName pqn;UA_QualifiedName_init(&pqn);
            UA_Server_readBrowseName(parent->master->master, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), &pqn);
            UA_BrowsePathResult property = node_browse_path(parent->master->master, child_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), pqn, false);
            UA_QualifiedName_clear(&pqn);

            VALUE blk;
            if (child_id.identifierType == UA_NODEIDTYPE_NUMERIC) {
              blk = rb_hash_aref(parent->master->methods,INT2NUM(child_id.identifier.numeric));
            } else if (child_id.identifierType == UA_NODEIDTYPE_STRING) {
              blk = rb_hash_aref(parent->master->methods,rb_str_new((const char *)child_id.identifier.string.data,child_id.identifier.string.length));
            } else if (child_id.identifierType == UA_NODEIDTYPE_BYTESTRING) {
              blk = rb_hash_aref(parent->master->methods,rb_str_new((const char *)child_id.identifier.byteString.data,child_id.identifier.byteString.length));
            } else {
              blk = Qnil;
            }


            if (property.statusCode == UA_STATUSCODE_GOOD) {
              node_add_variable_ua(UA_NS0ID_PROPERTYTYPE,UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,al,blk);
            } else {
              node_add_variable_ua(UA_NS0ID_BASEDATAVARIABLETYPE,UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,al,blk);
            }

            UA_BrowsePathResult_clear(&property);
          }
          if(nc == UA_NODECLASS_METHOD) {
            UA_NodeId ia;
            UA_NodeId oa;
            VALUE blk = rb_hash_aref(parent->master->methods,INT2NUM(child_id.identifier.numeric));

            bool iacheck = server_node_get_reference_by_name(parent->master->master, child_id, UA_QUALIFIEDNAME(0,"InputArguments"), &ia, false);
            bool oacheck = server_node_get_reference_by_name(parent->master->master, child_id, UA_QUALIFIEDNAME(0,"OutputArguments"), &oa, false);
            if (iacheck && oacheck) {
              UA_Variant arv1; UA_Variant_init(&arv1);
              UA_Variant arv2; UA_Variant_init(&arv2);
              UA_Server_readValue(parent->master->master, ia, &arv1);
              UA_Server_readValue(parent->master->master, oa, &arv2);

              // todo differentiate between input and output reference
              node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,arv1.arrayLength,(UA_Argument *)arv1.data,arv2.arrayLength,(UA_Argument *)arv2.data,blk);
              UA_Variant_clear(&arv1);
              UA_Variant_clear(&arv2);
            } else if (iacheck) {
              UA_Variant arv; UA_Variant_init(&arv);
              UA_Server_readValue(parent->master->master, ia, &arv);

              // todo differentiate between input and output reference
              node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,arv.arrayLength,(UA_Argument *)arv.data,0,NULL,blk);
              UA_Variant_clear(&arv);
            } else if (oacheck) {
              UA_Variant arv; UA_Variant_init(&arv);
              UA_Server_readValue(parent->master->master, oa, &arv);

              // todo differentiate between input and output reference
              node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,0,NULL,arv.arrayLength,(UA_Argument *)arv.data,blk);
              UA_Variant_clear(&arv);
            } else {
              node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns,buffer),dn,qn,newnode,0,NULL,0,NULL,blk);
            }
          }
        }
        UA_BrowsePathResult_clear(&mandatory);
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

  if (!(rb_obj_is_kind_of(parent,cTypeTopNode) || rb_obj_is_kind_of(parent,cTypeSubNode))) {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");
  Data_Get_Struct(parent, node_struct, ts);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Target node does not exist anymore.");

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

  UA_NodeId n = UA_NODEID_STRING(ns->master->default_ns, nidstr);
  node_struct *ret = node_alloc(ns->master,n);
  node_struct *handle[2] = { ts, ret };

  node_add_object_ua_rec(n,UA_LOCALIZEDTEXT("en-US", nstr),UA_QUALIFIEDNAME(ns->master->default_ns, nstr),ns,ts,ts->id,node_manifest_iter,(void *)handle);

  return Data_Wrap_Struct(CLASS_OF(self), node_mark, node_free, ret);
} //}}}

static VALUE node_find(VALUE self, VALUE qname) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(qname);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_BrowsePathResult bpr = node_browse_path(ns->master->master, ns->id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(ns->master->default_ns, nstr), false);

  if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
    return Qnil;
  } else {
    UA_NodeId ret;UA_NodeId_init(&ret);
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId,&ret);
    UA_BrowsePathResult_clear(&bpr);

    UA_NodeClass nc;UA_NodeClass_init(&nc);
    UA_Server_readNodeClass(ns->master->master, ret, &nc);

    VALUE node;

    if (nc == UA_NODECLASS_VARIABLE) {
      node = node_wrap(cVarNode,node_alloc(ns->master,ret));
    } else if (nc == UA_NODECLASS_METHOD) {
      node = node_wrap(cMethodNode,node_alloc(ns->master,ret));
    } else {
      node = node_wrap(cObjectNode,node_alloc(ns->master,ret));
    }
    UA_NodeClass_clear(&nc);

    return node;
  }
} //}}}
static VALUE server_get(int argc, VALUE* argv, VALUE self) { //{{{
  if (argc > 2 || argc < 1) { // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  VALUE ns = UINT2NUM(pss->default_ns);
  VALUE id;

  if (argc == 1) {
    id = argv[0];
  } else {
    ns = argv[0];
    id = argv[1];
  }

  if (NIL_P(ns) || TYPE(ns) != T_FIXNUM)
    rb_raise(rb_eTypeError, "ns is not a valid (numeric) namespace id");

  UA_NodeId it;

  if (TYPE(id) == T_FIXNUM) {
    it = UA_NODEID_NUMERIC(NUM2INT(ns), NUM2INT(id));
  } else {
    VALUE str = rb_obj_as_string(id);
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert url to string");
    char *nstr = (char *)StringValuePtr(str);

    it = UA_NODEID_STRING(NUM2INT(ns), nstr);
  }

  UA_NodeClass nc;UA_NodeClass_init(&nc);
  UA_Server_readNodeClass(pss->master, it, &nc);

  VALUE node;
  if (nc == UA_NODECLASS_VARIABLE) {
    node = node_wrap(cVarNode,node_alloc(pss, it));
  } else if (nc == UA_NODECLASS_METHOD) {
    node = node_wrap(cNode,node_alloc(pss, it));
  } else if (nc == UA_NODECLASS_UNSPECIFIED) {
    node = Qnil;
  } else {
    node = node_wrap(cNode,node_alloc(pss, it));
  }
  UA_NodeClass_clear(&nc);

  return node;
} //}}}

static VALUE node_value_set(VALUE self, VALUE value) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_Variant variant;
  if (value_to_variant(value,&variant,-1)) {
    // printf("-----------------------------------------%ld\n",variant.arrayDimensionsSize);
    if (variant.arrayDimensionsSize > 0) {
      UA_Server_writeValueRank(ns->master->master, ns->id, variant.arrayDimensionsSize);
      UA_Variant uaArrayDimensions;
      UA_Variant_setArray(&uaArrayDimensions, variant.arrayDimensions, variant.arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
      UA_Server_writeArrayDimensions(ns->master->master, ns->id, uaArrayDimensions);
    }

    UA_Server_writeValue(ns->master->master, ns->id, variant);
    UA_Variant_clear(&variant);
  }
  return self;
} //}}}
static VALUE node_value(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(ns->master->master, ns->id, &value);


  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  return rb_ary_entry(ret,0);
} //}}}
static VALUE node_delete(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_StatusCode retval = UA_Server_deleteNode(ns->master->master, ns->id, true);

  if (retval == UA_STATUSCODE_GOOD) {
    ns->exists = false;
    return Qtrue;
  }

  return Qfalse;
} //}}}
static VALUE node_exists(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (ns->exists)
    return Qtrue;
  else
    return Qfalse;
} //}}}
static VALUE node_description_set(VALUE self, VALUE value) { //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(value);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);
  UA_LocalizedText lt = UA_LOCALIZEDTEXT("en-US",nstr);

  UA_Server_writeDescription(ns->master->master, ns->id, lt);
  return self;
} //}}}
static VALUE node_description(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists) rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_LocalizedText value;
  UA_LocalizedText_init(&value);
  UA_StatusCode retval = UA_Server_readDescription(ns->master->master, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    ret = rb_str_export_locale(rb_str_new((char *)(value.text.data),value.text.length));
  }

  UA_LocalizedText_clear(&value);
  return ret;
} //}}}

/* -- */
static void  server_free(server_struct *pss) { //{{{
  if (pss != NULL) {
    if (!NIL_P(pss->methods)) {
      rb_gc_unregister_address(&pss->methods);
    }
    UA_Server_delete(pss->master);
    free(pss);
  }
} //}}}
static VALUE server_alloc(VALUE self) { //{{{
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->master = UA_Server_new();
  pss->config = UA_Server_getConfig(pss->master);
  pss->default_ns = 1;
  pss->debug = true;
  pss->methods = rb_hash_new();

  rb_gc_register_address(&pss->methods);

  UA_ServerConfig_setDefault(pss->config);

	return Data_Wrap_Struct(self, NULL, server_free, pss);
} //}}}
/* ++ */

static VALUE server_init(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->master);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
} //}}}
static VALUE server_run(VALUE self) { //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->master, false);

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

  pss->default_ns = UA_Server_addNamespace(pss->master, nstr);
  return INT2NUM(pss->default_ns);
} //}}}
static VALUE server_types(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cTypeTopNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE)));
} //}}}
static VALUE server_references(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cReferenceTopNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES)));
} //}}}
static VALUE server_objects(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cObjectNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)));
} //}}}
static VALUE server_debug(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  return (pss->debug) ? Qtrue : Qfalse;
} //}}}
static VALUE server_debug_set(VALUE self, VALUE val) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  if (val == Qtrue) {
    pss->config->logger = UA_Log_Stdout_;
    pss->debug = Qtrue;
  } else {
    pss->config->logger = UA_Log_None_;
    pss->debug = Qfalse;
  }
  return self;
} //}}}
static VALUE server_namespaces(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(pss->master, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  RB_OBJ_FREEZE(ret);
  return rb_ary_entry(ret,0);
} //}}}
static VALUE server_active_namespace(VALUE self) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return UINT2NUM(pss->default_ns);
} //}}}
static VALUE server_active_namespace_set(VALUE self, VALUE val) { //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  if (NIL_P(val) || TYPE(val) != T_FIXNUM)
    rb_raise(rb_eTypeError, "namespace is not an integer");

  pss->default_ns = NUM2UINT(val);
  return self;
} //}}}

void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");
  rb_define_const(mOPCUA, "MANDATORY", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORY));
  rb_define_const(mOPCUA, "MANDATORYPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORYPLACEHOLDER));
  rb_define_const(mOPCUA, "OPTIONAL", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONAL));
  rb_define_const(mOPCUA, "OPTIONALPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONALPLACEHOLDER));
  rb_define_const(mOPCUA, "BASEDATAVARIABLETYPE", INT2NUM(UA_NS0ID_BASEDATAVARIABLETYPE));
  rb_define_const(mOPCUA, "PROPERTYTYPE", INT2NUM(UA_NS0ID_PROPERTYTYPE));

  Init_types(mOPCUA);

  cServer            = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cNode              = rb_define_class_under(cServer, "Node", rb_cObject);
  cObjectNode        = rb_define_class_under(cServer, "ObjectNode", cNode);
  cTypeTopNode       = rb_define_class_under(cServer, "TypeTopNode", cNode);
  cTypeSubNode       = rb_define_class_under(cServer, "TypeSubNode", cNode);
  cReferenceTopNode  = rb_define_class_under(cServer, "ReferenceTopNode", cNode);
  cReferenceSubNode  = rb_define_class_under(cServer, "ReferenceSubNode", cNode);
  cReferenceNode     = rb_define_class_under(cServer, "ObjectReferenceNode", cNode);
  cVarNode           = rb_define_class_under(cServer, "ObjectVarNode", cNode);
  cMethodNode        = rb_define_class_under(cServer, "ObjectMethodNode", cNode);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", server_run, 0);
  rb_define_method(cServer, "add_namespace", server_add_namespace, 1);
  rb_define_method(cServer, "active_namespace", server_active_namespace, 0);
  rb_define_method(cServer, "active_namespace=", server_active_namespace_set, 1);
  rb_define_method(cServer, "types", server_types, 0);
  rb_define_method(cServer, "references", server_references, 0);
  rb_define_method(cServer, "objects", server_objects, 0);
  rb_define_method(cServer, "debug", server_debug, 0);
  rb_define_method(cServer, "debug=", server_debug_set, 1);
  rb_define_method(cServer, "namespaces", server_namespaces, 0);
  rb_define_method(cServer, "get", server_get, -1);

  rb_define_method(cNode, "to_s", node_to_s, 0);
  rb_define_method(cNode, "id", node_id, 0);
  rb_define_method(cNode, "name", node_name, 0);
  rb_define_method(cNode, "description", node_description, 0);
  rb_define_method(cNode, "description=", node_description_set, 1);
  rb_define_method(cNode, "exists?", node_exists, 0);

  rb_define_method(cTypeTopNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypeTopNode, "add_reference_type", node_add_reference_type, 2);
  rb_define_method(cTypeTopNode, "folder", node_type_folder, 0);

  rb_define_method(cTypeSubNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypeSubNode, "add_variable", node_add_variable, -1);
  rb_define_method(cTypeSubNode, "add_variable_rw", node_add_variable_rw, -1);
  rb_define_method(cTypeSubNode, "add_object", node_add_object, -1);
  rb_define_method(cTypeSubNode, "add_method", node_add_method, -1);
  rb_define_method(cTypeSubNode, "add_reference", node_add_reference, 2);
  rb_define_method(cTypeSubNode, "delete!", node_delete, 0);

  rb_define_method(cObjectNode, "manifest", node_manifest, 2);
  rb_define_method(cObjectNode, "find", node_find, 1);
  rb_define_method(cObjectNode, "delete!", node_delete, 0);

  rb_define_method(cVarNode, "value", node_value, 0);
  rb_define_method(cVarNode, "value=", node_value_set, 1);
  rb_define_method(cVarNode, "delete!", node_delete, 0);
}
