#include "server.h"
#include <string.h>
#include <stdlib.h>

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
VALUE cReferenceTypeNode = Qnil;
VALUE cVariableTypeNode = Qnil;
VALUE cDataTypeNode = Qnil;

#include "values.h"

int nodecounter = 2000;

/* -- */
static void node_free(node_struct *ns)
{ //{{{
  if (ns != NULL)
  {
    if (!NIL_P(ns->method))
    {
      rb_gc_unregister_address(&ns->method);
    }
    free(ns);
  }
} // }}}
static node_struct *node_alloc(server_struct *server, UA_NodeId nodeid)
{ //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->master = server;
  ns->id = nodeid;
  ns->method = Qnil;
  ns->exists = true;

  return ns;
} //}}}
static VALUE node_wrap(VALUE klass, node_struct *ns)
{ //{{{
  return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */

static VALUE node_type_folder(VALUE self)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");
  return node_wrap(cTypeTopNode, node_alloc(ns->master, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE)));
} //}}}
static VALUE node_add_object_type(VALUE self, VALUE name)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

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

  return node_wrap(cTypeSubNode, node_alloc(ns->master, n));
} //}}}
static VALUE node_add_reference_type(VALUE self, VALUE name, VALUE type)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

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

  return node_wrap(cReferenceSubNode, node_alloc(ns->master, n));
} //}}}

static UA_NodeId nodeid_from_str(VALUE nodeid)
{ //{{{
  VALUE str = rb_obj_as_string(nodeid);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);
  int nid_index = 0, index = 2;
  char nid_type, *nid_id, *with_ns;
  with_ns = strchr(nstr, ';'); //get offset of ';'
  if (with_ns == NULL)         // nodeid looks like 'i=45' (equals 'ns=0;i=45')
  {
    nid_id = (char *)calloc(strlen(nstr) - 2, sizeof(char));
    strncpy(nid_id, nstr + 2, strlen(nstr) - 2);
    nid_id[strlen(nstr) - 2] = '\0';
    nid_type = nstr[0];
  }
  else // nodeid looks like 'ns=0;i=45'
  {
    index = (int)(with_ns - nstr);
    char *nsi = (char *)calloc(index - 3, sizeof(char));
    strncpy(nsi, nstr + 3, index - 3);
    nid_index = atoi(nsi);
    free(nsi);
    nid_type = nstr[index + 1];
    nid_id = calloc(strlen(nstr) - index - 3, sizeof(char));
    strncpy(nid_id, nstr + index + 3, strlen(nstr) - index - 3);
  }

  if (nid_type == 'i')
  {
    //printf("'%s' to 'ns=%d;i=%i'\n", nstr, nid_index, atoi(nid_id));
    UA_NodeId id = UA_NODEID_NUMERIC(nid_index, atoi(nid_id));
    free(nid_id);
    return id;
  }
  else if (nid_type == 's')
  {
    //printf("'%s' to 'ns=%d;s=%s'\n", nstr, nid_index, nid_id);
    return UA_NODEID_STRING(nid_index, nid_id);
  }
  else
  {
    //printf("'%s' to 'ns=%d;g=%s'\n", nstr, nid_index, nid_id);
    // TODO: add GUID,...
    return UA_NODEID_STRING(nid_index, nid_id);
  }
} //}}}

static VALUE server_add_reference_type(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference, VALUE symmetric)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  int sym = RTEST(symmetric);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_ReferenceTypeAttributes rtAttr = UA_ReferenceTypeAttributes_default;
  rtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  rtAttr.symmetric = sym;
  UA_Server_addReferenceTypeNode(pss->master,
                                 nid,
                                 pa->id,
                                 re->id,
                                 UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                                 rtAttr,
                                 NULL,
                                 NULL);

  return node_wrap(cReferenceTypeNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_data_type(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_DataTypeAttributes dtAttr = UA_DataTypeAttributes_default;
  dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addDataTypeNode(pss->master,
                            nid,
                            pa->id,
                            re->id,
                            UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                            dtAttr,
                            NULL,
                            NULL);

  return node_wrap(cDataTypeNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_variable_type(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference, VALUE datatype, VALUE dimensions)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);
  node_struct *dt;
  Data_Get_Struct(datatype, node_struct, dt);

  int size = RARRAY_LEN(dimensions);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
  vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  vtAttr.dataType = dt->id;
  if (size > 0)
  {
    UA_UInt32 *dims = (UA_UInt32 *)malloc(size * sizeof(UA_UInt32));
    for (long i = 0; i < size; i++)
    {
      dims[i] = NUM2UINT(rb_ary_entry(dimensions, i));
    }
    vtAttr.valueRank = size; //possibly false if size > 3
    vtAttr.arrayDimensions = dims;
    vtAttr.arrayDimensionsSize = size;
  }

  UA_Server_addVariableTypeNode(pss->master,
                                nid,
                                pa->id,
                                re->id,
                                UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                                UA_NODEID_NULL,
                                vtAttr,
                                NULL,
                                NULL);

  return node_wrap(cVariableTypeNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_object_type(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
  dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectTypeNode(pss->master,
                              nid,
                              pa->id,
                              re->id,
                              UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                              dtAttr,
                              NULL,
                              NULL);

  return node_wrap(cTypeSubNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_object(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference, VALUE type)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);
  node_struct *ty;
  Data_Get_Struct(type, node_struct, ty);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
  oAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  UA_Server_addObjectNode(pss->master,
                          nid,
                          pa->id,
                          re->id,
                          UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                          ty->id,
                          oAttr,
                          NULL,
                          NULL);

  return node_wrap(cObjectNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_method(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_MethodAttributes mAttr = UA_MethodAttributes_default;
  mAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  mAttr.executable = true;
  mAttr.userExecutable = true;
  UA_Server_addMethodNode(pss->master,
                          nid,
                          pa->id,
                          re->id,
                          UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                          mAttr,
                          NULL,
                          0,
                          NULL,
                          0,
                          NULL,
                          NULL,
                          NULL);

  return node_wrap(cMethodNode, node_alloc(pss, nid));
} //}}}
static VALUE server_add_variable(VALUE self, VALUE name, VALUE nodeid, VALUE parent, VALUE reference, VALUE type)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  node_struct *pa;
  Data_Get_Struct(parent, node_struct, pa);
  node_struct *re;
  Data_Get_Struct(reference, node_struct, re);
  node_struct *ty;
  Data_Get_Struct(type, node_struct, ty);

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_NodeId nid = nodeid_from_str(nodeid);

  UA_UInt32 mask;
  UA_UInt32_init(&mask);
  UA_Server_readWriteMask(pss->master, ty->id, &mask);
  UA_Int32 rank;
  UA_Int32_init(&rank);
  UA_Server_readValueRank(pss->master, ty->id, &rank);
  UA_Variant dim;
  UA_Variant_init(&dim);
  UA_Server_readArrayDimensions(pss->master, ty->id, &dim);
  UA_NodeId datatype;
  UA_NodeId_init(&datatype);
  UA_Server_readDataType(pss->master, ty->id, &datatype);

  UA_VariableAttributes vAttr = UA_VariableAttributes_default;
  vAttr.displayName = UA_LOCALIZEDTEXT("en-US", nstr);
  vAttr.writeMask = mask;
  vAttr.valueRank = rank;
  vAttr.dataType = datatype;
  vAttr.arrayDimensions = (UA_UInt32 *)dim.data;
  vAttr.arrayDimensionsSize = dim.arrayLength;
  UA_Server_addVariableNode(pss->master,
                            nid,
                            pa->id,
                            re->id,
                            UA_QUALIFIEDNAME(nid.namespaceIndex, nstr),
                            ty->id,
                            vAttr,
                            NULL,
                            NULL);

  UA_UInt32_clear(&mask);
  UA_Int32_clear(&rank);
  UA_Variant_clear(&dim);
  UA_NodeId_clear(&datatype);
  return node_wrap(cVarNode, node_alloc(pss, nid));
} //}}}
static VALUE node_id(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE ret = rb_ary_new();

  rb_ary_push(ret, UINT2NUM(ns->id.namespaceIndex));
  if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC)
  {
    VALUE id = UINT2NUM((UA_UInt32)(ns->id.identifier.numeric));
    rb_ary_push(ret, id);
  }
  else if (ns->id.identifierType == UA_NODEIDTYPE_STRING)
  {
    rb_ary_push(ret, rb_str_new((const char *)ns->id.identifier.string.data, ns->id.identifier.string.length));
  }
  else if (ns->id.identifierType == UA_NODEIDTYPE_BYTESTRING)
  {
    rb_ary_push(ret, rb_str_new((const char *)ns->id.identifier.byteString.data, ns->id.identifier.byteString.length));
  }
  return ret;
} //}}}
static VALUE node_name(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_QualifiedName qn;
  UA_QualifiedName_init(&qn);
  UA_Server_readBrowseName(ns->master->master, ns->id, &qn);

  return rb_sprintf("%.*s", (int)qn.name.length, qn.name.data);
} //}}}
static VALUE node_to_s(VALUE self)
{ //{{{
  node_struct *ns;
  VALUE ret;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  if (ns->id.identifierType == UA_NODEIDTYPE_NUMERIC)
  {
    ret = rb_sprintf("ns=%d;i=%d", ns->id.namespaceIndex, ns->id.identifier.numeric);
  }
  else if (ns->id.identifierType == UA_NODEIDTYPE_STRING)
  {
    ret = rb_sprintf("ns=%d;s=%.*s", ns->id.namespaceIndex, (int)ns->id.identifier.string.length, ns->id.identifier.string.data);
  }
  else
  {
    ret = rb_sprintf("ns=%d;unsupported", ns->id.namespaceIndex);
  }
  return ret;
} //}}}
static VALUE node_add_reference(VALUE self, VALUE to, VALUE type)
{ //{{{
  node_struct *ns;
  node_struct *tos;
  node_struct *tys;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  if (!(rb_obj_is_kind_of(type, cReferenceSubNode) || rb_obj_is_kind_of(to, cTypeSubNode)))
  {
    rb_raise(rb_eArgError, "arguments have to be NodeIDs.");
  }
  Data_Get_Struct(to, node_struct, tos);
  if (!tos->exists)
    rb_raise(rb_eRuntimeError, "To (arg 1) node does not exist anymore.");
  Data_Get_Struct(type, node_struct, tys);
  if (!tys->exists)
    rb_raise(rb_eRuntimeError, "Type (arg 2) node does not exist anymore.");
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

  return node_wrap(cReferenceNode, node_alloc(ns->master, n));
} //}}}

static UA_StatusCode node_add_method_callback( //{{{
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *methodId, void *methodContext,
    const UA_NodeId *objectId, void *objectContext,
    size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output)
{
  node_struct *me = (node_struct *)methodContext;

  // printf(
  //   "NodeId %d, %-16.*s\n",
  //   me->id.namespaceIndex,
  //   (int)me->id.identifier.string.length,
  //   me->id.identifier.string.data
  // );

  VALUE args = rb_ary_new();
  rb_ary_push(args, Data_Wrap_Struct(cObjectNode, NULL, NULL, me));
  for (int i = 0; i < inputSize; i++)
  {
    VALUE ret = extract_value(input[i]);
    rb_ary_push(args, rb_ary_entry(ret, 0));
  }

  rb_proc_call(me->method, args);

  return UA_STATUSCODE_GOOD;
} //}}}
static UA_NodeId node_add_method_ua(UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, size_t inputArgumentsSize, const UA_Argument *inputArguments, VALUE blk)
{ //{{{
  UA_MethodAttributes mnAttr = UA_MethodAttributes_default;
  mnAttr.displayName = dn;
  mnAttr.executable = true;
  mnAttr.userExecutable = true;

  node_struct *me = node_alloc(parent->master, n);
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
                          0,
                          NULL,
                          (void *)me,
                          NULL);

  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);

  return n;
} //}}}
static UA_NodeId node_add_method_ua_simple(char *nstr, node_struct *parent, VALUE opts, VALUE blk)
{ //{{{
  UA_Argument inputArguments[RHASH_SIZE(opts)];

  VALUE ary = rb_funcall(opts, rb_intern("to_a"), 0);
  for (long i = 0; i < RARRAY_LEN(ary); i++)
  {
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
  int nodeid = nodecounter++;

  rb_hash_aset(parent->master->methods, INT2NUM(nodeid), blk);
  rb_gc_register_address(&blk);

  return node_add_method_ua(
      UA_NODEID_NUMERIC(parent->master->default_ns, nodeid),
      UA_LOCALIZEDTEXT("en-US", nstr),
      UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
      parent,
      RHASH_SIZE(opts),
      inputArguments,
      blk);
} //}}}
static VALUE node_add_method(int argc, VALUE *argv, VALUE self)
{ //{{{
  node_struct *parent;

  VALUE name;
  VALUE opts;
  VALUE blk;
  rb_gc_register_address(&blk);

  if (argc < 1)
  { // there should be 1 or more arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }
  rb_scan_args(argc, argv, "1:&", &name, &opts, &blk);
  if (NIL_P(opts))
    opts = rb_hash_new();

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self), node_alloc(parent->master, node_add_method_ua_simple(nstr, parent, opts, blk)));
} //}}}

static UA_NodeId node_add_variable_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, UA_Byte accesslevelmask)
{ //{{{
  UA_VariableAttributes vAttr = UA_VariableAttributes_default;
  vAttr.displayName = dn;
  vAttr.accessLevel = accesslevelmask;

  UA_Server_addVariableNode(parent->master->master,
                            n,
                            parent->id,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            qn,
                            UA_NODEID_NUMERIC(0, type),
                            vAttr,
                            NULL,
                            NULL);

  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);

  return n;
} //}}}
static UA_NodeId node_add_variable_ua_simple(UA_Int32 type, char *nstr, node_struct *parent, UA_Byte accesslevelmask, bool numeric)
{ //{{{
  return node_add_variable_ua(
      type,
      numeric ? UA_NODEID_NUMERIC(parent->master->default_ns, nodecounter++) : UA_NODEID_STRING(parent->master->default_ns, nstr),
      UA_LOCALIZEDTEXT("en-US", nstr),
      UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
      parent,
      accesslevelmask);
} //}}}
static VALUE node_add_variable_wrap(int argc, VALUE *argv, VALUE self, UA_Byte accesslevelmask, bool numeric)
{ //{{{
  node_struct *parent;

  if (argc > 2 || argc == 0)
  { // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 2 && argv[1] != Qnil)
  {
    type = NUM2INT(argv[1]);
  }
  else
  {
    type = UA_NS0ID_BASEDATAVARIABLETYPE;
  }

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(cVarNode, node_alloc(parent->master, node_add_variable_ua_simple(type, nstr, parent, accesslevelmask, numeric)));
} //}}}
static VALUE node_add_variable(int argc, VALUE *argv, VALUE self)
{ //{{{
  return node_add_variable_wrap(argc, argv, self, UA_ACCESSLEVELMASK_READ, true);
} //}}}
static VALUE node_add_variable_rw(int argc, VALUE *argv, VALUE self)
{ //{{{
  return node_add_variable_wrap(argc, argv, self, UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE, true);
} //}}}

static void node_add_object_ua_rec(UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, node_struct *datatype, UA_NodeId cid, UA_NodeIteratorCallback callback, void *handle)
{ //{{{
  UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
  oAttr.displayName = dn;
  UA_Server_addNode_begin(parent->master->master,
                          UA_NODECLASS_OBJECT,
                          n,
                          parent->id,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          qn,
                          datatype->id,
                          (const UA_NodeAttributes *)&oAttr,
                          &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                          NULL,
                          NULL);

  // printf("---->\n");
  UA_Server_forEachChildNodeCall(parent->master->master, cid, callback, handle);
  // printf("<----\n");

  UA_Server_addNode_finish(parent->master->master, n);
  UA_Server_addReference(parent->master->master,
                         n,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                         true);
} //}}}
static UA_NodeId node_add_object_ua(UA_Int32 type, UA_NodeId n, UA_LocalizedText dn, UA_QualifiedName qn, node_struct *parent, node_struct *datatype, VALUE ref)
{ //{{{
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

  if (ref != Qnil)
  {
    UA_Server_addReference(parent->master->master,
                           n,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, type),
                           true);
  }

  return n;
} //}}}
static UA_NodeId node_add_object_ua_simple(UA_Int32 type, char *nstr, node_struct *parent, node_struct *datatype, VALUE ref)
{ //{{{
  return node_add_object_ua(
      type,
      UA_NODEID_STRING(parent->master->default_ns, nstr),
      UA_LOCALIZEDTEXT("en-US", nstr),
      UA_QUALIFIEDNAME(parent->master->default_ns, nstr),
      parent,
      datatype,
      ref);
} //}}}
static VALUE node_add_object(int argc, VALUE *argv, VALUE self)
{ //{{{
  node_struct *parent;
  node_struct *datatype;

  if (argc > 3 || argc < 2)
  { // there should only be 2 or 3 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  UA_UInt32 type;
  if (argc == 3 && argv[2] != Qnil)
  {
    type = NUM2INT(argv[2]);
  }
  else
  {
    type = UA_NS0ID_MODELLINGRULE_MANDATORY;
  }

  if (!(rb_obj_is_kind_of(argv[1], cTypeTopNode) || rb_obj_is_kind_of(argv[1], cTypeSubNode)))
  {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, parent);
  if (!parent->exists)
    rb_raise(rb_eRuntimeError, "Parent node does not exist anymore.");
  Data_Get_Struct(argv[1], node_struct, datatype);
  if (!datatype->exists)
    rb_raise(rb_eRuntimeError, "Datatype node does not exist anymore.");

  VALUE str = rb_obj_as_string(argv[0]);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  return node_wrap(CLASS_OF(self), node_alloc(parent->master, node_add_object_ua_simple(type, nstr, parent, datatype, argv[2])));
} //}}}

static VALUE node_follow(VALUE self, VALUE reference_type, VALUE direction)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  node_struct *rt;
  Data_Get_Struct(reference_type, node_struct, rt);

  int dir = NUM2INT(direction);

  UA_BrowseDescription bDes;
  UA_BrowseDescription_init(&bDes);
  bDes.nodeId = ns->id;
  bDes.resultMask = UA_BROWSERESULTMASK_ALL;
  bDes.referenceTypeId = rt->id;
  bDes.browseDirection = dir;

  UA_BrowseResult bRes = UA_Server_browse(ns->master->master, 999, &bDes);

  VALUE nodes = rb_ary_new();
  for (int i = 0; i < bRes.referencesSize; i++)
  {
    UA_ReferenceDescription *ref = &(bRes.references[i]);

    UA_NodeClass nc;
    UA_NodeClass_init(&nc);
    UA_Server_readNodeClass(ns->master->master, ref->nodeId.nodeId, &nc);

    VALUE node;
    if (nc == UA_NODECLASS_VARIABLE)
    {
      node = node_wrap(cVarNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_METHOD)
    {
      node = node_wrap(cNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_UNSPECIFIED)
    {
      node = Qnil;
    }
    else if (nc == UA_NODECLASS_OBJECTTYPE)
    {
      node = node_wrap(cTypeTopNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_OBJECT)
    {
      node = node_wrap(cObjectNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_REFERENCETYPE)
    {
      node = node_wrap(cReferenceTypeNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_DATATYPE)
    {
      node = node_wrap(cDataTypeNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else if (nc == UA_NODECLASS_VARIABLETYPE)
    {
      node = node_wrap(cVariableTypeNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    else
    {
      node = node_wrap(cNode, node_alloc(ns->master, ref->nodeId.nodeId));
    }
    UA_NodeClass_clear(&nc);
    rb_ary_push(nodes, node);
  }
  UA_BrowseResult_deleteMembers(&bRes);
  UA_BrowseResult_clear(&bRes);

  return nodes;
} //}}}

static UA_StatusCode node_manifest_iter(UA_NodeId child_id, UA_Boolean is_inverse, UA_NodeId reference_type_id, void *handle)
{ //{{{
  if (is_inverse)
    return UA_STATUSCODE_GOOD;

  node_struct **tandle = (node_struct **)handle;
  node_struct *parent = tandle[0];
  node_struct *newnode = tandle[1];

  if (child_id.namespaceIndex == parent->master->default_ns)
  {
    UA_NodeClass nc;
    UA_NodeClass_init(&nc);

    UA_LocalizedText dn;
    UA_LocalizedText_init(&dn);
    UA_QualifiedName qn;
    UA_QualifiedName_init(&qn);
    UA_QualifiedName pqn;
    UA_QualifiedName_init(&pqn);
    UA_QualifiedName nqn;
    UA_QualifiedName_init(&nqn);

    UA_Byte al;
    UA_Byte_init(&al);

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

    if (child_id.namespaceIndex == parent->master->default_ns)
    {
      UA_QualifiedName mqn;
      UA_QualifiedName_init(&mqn);
      UA_Server_readBrowseName(parent->master->master, UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), &mqn);
      UA_BrowsePathResult mandatory = node_browse_path(parent->master->master, child_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE), mqn, false);
      UA_QualifiedName_clear(&mqn);

      if (mandatory.statusCode == UA_STATUSCODE_GOOD && (nc == UA_NODECLASS_OBJECT || nc == UA_NODECLASS_VARIABLE || nc == UA_NODECLASS_METHOD))
      {
        char *buffer = strnautocat(NULL, "", 0);
        if (newnode->id.identifier.string.data[0] != '/')
        {
          buffer = strnautocat(buffer, "/", 1);
        }
        buffer = strnautocat(buffer, (char *)newnode->id.identifier.string.data, newnode->id.identifier.string.length);
        buffer = strnautocat(buffer, "/", 1);
        buffer = strnautocat(buffer, (char *)qn.name.data, qn.name.length);
        if (nc == UA_NODECLASS_OBJECT)
        {
          UA_NodeId typeid;
          server_node_get_reference(parent->master->master, child_id, &typeid, false);

          node_struct *thetype = node_alloc(parent->master, typeid);

          UA_NodeId n = UA_NODEID_STRING(parent->master->default_ns, buffer);
          node_struct *downnode = node_alloc(parent->master, n);
          node_struct *newparent = node_alloc(parent->master, child_id);
          node_struct *downhandle[2] = {newparent, downnode};

          node_add_object_ua_rec(UA_NODEID_STRING(parent->master->default_ns, buffer), dn, qn, newnode, thetype, child_id, node_manifest_iter, (void *)downhandle);

          free(thetype);
          free(downnode);
          free(newparent);
        }
        if (nc == UA_NODECLASS_VARIABLE)
        {
          UA_QualifiedName pqn;
          UA_QualifiedName_init(&pqn);
          UA_Server_readBrowseName(parent->master->master, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), &pqn);
          UA_BrowsePathResult property = node_browse_path(parent->master->master, child_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), pqn, false);
          UA_QualifiedName_clear(&pqn);

          if (property.statusCode == UA_STATUSCODE_GOOD)
          {
            node_add_variable_ua(UA_NS0ID_PROPERTYTYPE, UA_NODEID_STRING(parent->master->default_ns, buffer), dn, qn, newnode, al);
          }
          else
          {
            node_add_variable_ua(UA_NS0ID_BASEDATAVARIABLETYPE, UA_NODEID_STRING(parent->master->default_ns, buffer), dn, qn, newnode, al);
          }

          UA_BrowsePathResult_clear(&property);
        }
        if (nc == UA_NODECLASS_METHOD)
        {
          UA_NodeId ttt;
          VALUE blk = rb_hash_aref(parent->master->methods, INT2NUM(child_id.identifier.numeric));
          if (server_node_get_reference(parent->master->master, child_id, &ttt, false))
          {
            UA_Variant arv;
            UA_Variant_init(&arv);
            UA_Server_readValue(parent->master->master, ttt, &arv);

            node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns, buffer), dn, qn, newnode, arv.arrayLength, (UA_Argument *)arv.data, blk);
            UA_Variant_clear(&arv);
          }
          else
          {
            node_add_method_ua(UA_NODEID_STRING(parent->master->default_ns, buffer), dn, qn, newnode, 0, NULL, blk);
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
static VALUE node_manifest(VALUE self, VALUE name, VALUE parent)
{ //{{{
  node_struct *ns;
  node_struct *ts;

  if (!(rb_obj_is_kind_of(parent, cTypeTopNode) || rb_obj_is_kind_of(parent, cTypeSubNode)))
  {
    rb_raise(rb_eArgError, "argument 2 has to be a type.");
  }

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");
  Data_Get_Struct(parent, node_struct, ts);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Target node does not exist anymore.");

  VALUE str = rb_obj_as_string(name);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  char *nidstr = strnautocat(NULL, "", 1);
  if (ns->id.identifierType == UA_NODEIDTYPE_STRING)
  {
    nidstr = strnautocat(nidstr, (char *)ns->id.identifier.string.data, ns->id.identifier.string.length);
    nidstr = strnautocat(nidstr, "/", 1);
  }
  nidstr = strnautocat(nidstr, nstr, strlen(nstr));

  UA_NodeId n = UA_NODEID_STRING(ns->master->default_ns, nidstr);
  node_struct *ret = node_alloc(ns->master, n);
  node_struct *handle[2] = {ts, ret};

  node_add_object_ua_rec(n, UA_LOCALIZEDTEXT("en-US", nstr), UA_QUALIFIEDNAME(ns->master->default_ns, nstr), ns, ts, ts->id, node_manifest_iter, (void *)handle);

  return Data_Wrap_Struct(CLASS_OF(self), NULL, node_free, ret);
} //}}}

static VALUE node_find(VALUE self, VALUE qname)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(qname);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_BrowsePathResult bpr = node_browse_path(ns->master->master, ns->id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(ns->master->default_ns, nstr), false);

  if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1)
  {
    return Qnil;
  }
  else
  {
    UA_NodeId ret;
    UA_NodeId_init(&ret);
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &ret);
    UA_BrowsePathResult_clear(&bpr);

    UA_NodeClass nc;
    UA_NodeClass_init(&nc);
    UA_Server_readNodeClass(ns->master->master, ret, &nc);

    VALUE node;

    if (nc == UA_NODECLASS_VARIABLE)
    {
      node = node_wrap(cVarNode, node_alloc(ns->master, ret));
    }
    else if (nc == UA_NODECLASS_METHOD)
    {
      node = node_wrap(cMethodNode, node_alloc(ns->master, ret));
    }
    else
    {
      node = node_wrap(cObjectNode, node_alloc(ns->master, ret));
    }
    UA_NodeClass_clear(&nc);

    return node;
  }
} //}}}
static VALUE server_get(int argc, VALUE *argv, VALUE self)
{ //{{{
  if (argc > 2 || argc < 1)
  { // there should only be 1 or 2 arguments
    rb_raise(rb_eArgError, "wrong number of arguments");
  }

  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  VALUE ns = UINT2NUM(pss->default_ns);
  VALUE id;

  if (argc == 1)
  {
    id = argv[0];
  }
  else
  {
    ns = argv[0];
    id = argv[1];
  }

  if (NIL_P(ns) || TYPE(ns) != T_FIXNUM)
    rb_raise(rb_eTypeError, "ns is not a valid (numeric) namespace id");

  UA_NodeId it;

  if (TYPE(id) == T_FIXNUM)
  {
    it = UA_NODEID_NUMERIC(NUM2INT(ns), NUM2INT(id));
  }
  else
  {
    VALUE str = rb_obj_as_string(id);
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert url to string");
    char *nstr = (char *)StringValuePtr(str);

    it = UA_NODEID_STRING(NUM2INT(ns), nstr);
  }

  UA_NodeClass nc;
  UA_NodeClass_init(&nc);
  UA_Server_readNodeClass(pss->master, it, &nc);

  VALUE node;
  if (nc == UA_NODECLASS_VARIABLE)
  {
    node = node_wrap(cVarNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_METHOD)
  {
    node = node_wrap(cNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_UNSPECIFIED)
  {
    node = Qnil;
  }
  else if (nc == UA_NODECLASS_OBJECTTYPE)
  {
    node = node_wrap(cTypeTopNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_OBJECT)
  {
    node = node_wrap(cObjectNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_REFERENCETYPE)
  {
    node = node_wrap(cReferenceTypeNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_DATATYPE)
  {
    node = node_wrap(cDataTypeNode, node_alloc(pss, it));
  }
  else if (nc == UA_NODECLASS_VARIABLETYPE)
  {
    node = node_wrap(cVariableTypeNode, node_alloc(pss, it));
  }
  else
  {
    node = node_wrap(cNode, node_alloc(pss, it));
  }
  UA_NodeClass_clear(&nc);

  return node;
} //}}}

static VALUE node_value_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_NodeId datatype_nid;
  UA_NodeId_init(&datatype_nid);
  UA_StatusCode retval = UA_Server_readDataType(ns->master->master, ns->id, &datatype_nid);

  int datatype_proposal = -1;
  if (retval == UA_STATUSCODE_GOOD && datatype_nid.namespaceIndex == 0 && datatype_nid.identifierType == UA_NODEIDTYPE_NUMERIC)
  {
    datatype_proposal = datatype_nid.identifier.numeric - 1; // -1 because open62541 datatype enum starts at 0, ua datatype nodeids at 1
  }
  UA_NodeId_clear(&datatype_nid);

  UA_Variant variant;
  if (value_to_variant(value, &variant, datatype_proposal))
  {
    if (variant.arrayDimensionsSize > 0)
    {
      UA_Variant dim;
      UA_Variant_init(&dim);
      UA_Server_readArrayDimensions(ns->master->master, ns->id, &dim);

      if (dim.arrayLength < 1)
      {
        //printf("Array dim <=1: %ld; Length: %ld\n", dim.arrayLength, variant.arrayLength);
        UA_Server_writeValueRank(ns->master->master, ns->id, 1);

        uint d[1] = {variant.arrayLength};
        UA_Variant uaArrayDimensions;
        UA_Variant_setArray(&uaArrayDimensions, d, 1, &UA_TYPES[UA_TYPES_UINT32]);
        UA_Server_writeArrayDimensions(ns->master->master, ns->id, uaArrayDimensions);
        //variant.arrayDimensions = d; //seems unnecessary and somehow creates matrix in uaexpert
        //variant.arrayDimensionsSize = 1;
      }
      else if (dim.arrayLength == 1)
      {
        //printf("Array dim =1: %ld; Length: %ld\n", dim.arrayLength, variant.arrayLength);
        uint d[1] = {variant.arrayLength};
        UA_Variant uaArrayDimensions;
        UA_Variant_setArray(&uaArrayDimensions, d, 1, &UA_TYPES[UA_TYPES_UINT32]);
        UA_Server_writeArrayDimensions(ns->master->master, ns->id, uaArrayDimensions);
      }
      else if (dim.arrayLength > 1)
      {
        //printf("Matrix  dim >1: %ld\n", dim.arrayLength);
        variant.arrayDimensions = (UA_UInt32 *)dim.data;
        variant.arrayDimensionsSize = dim.arrayLength;
      }

      UA_Server_writeValue(ns->master->master, ns->id, variant);

      //UA_Variant_clear(&dim); //do not clear here
    }
    else
    {
      UA_Server_writeValue(ns->master->master, ns->id, variant);
    }
  }
  return self;
} //}}}
static VALUE node_value(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(ns->master->master, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  return ret;
} //}}}
static VALUE node_delete(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_StatusCode retval = UA_Server_deleteNode(ns->master->master, ns->id, true);

  if (retval == UA_STATUSCODE_GOOD)
  {
    ns->exists = false;
    return Qtrue;
  }

  return Qfalse;
} //}}}
static VALUE node_exists(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (ns->exists)
    return Qtrue;
  else
    return Qfalse;
} //}}}
static VALUE node_description_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  VALUE str = rb_obj_as_string(value);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);
  UA_LocalizedText lt = UA_LOCALIZEDTEXT("en-US", nstr);

  UA_Server_writeDescription(ns->master->master, ns->id, lt);
  return self;
} //}}}
static VALUE node_description(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);
  if (!ns->exists)
    rb_raise(rb_eRuntimeError, "Node does not exist anymore.");

  UA_LocalizedText value;
  UA_LocalizedText_init(&value);
  UA_StatusCode retval = UA_Server_readDescription(ns->master->master, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = rb_str_export_locale(rb_str_new((char *)(value.text.data), value.text.length));
  }

  UA_LocalizedText_clear(&value);
  return ret;
} //}}}
static VALUE node_inversename_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  VALUE str = rb_obj_as_string(value);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert obj to string");
  char *nstr = (char *)StringValuePtr(str);
  UA_LocalizedText lt = UA_LOCALIZEDTEXT("en-US", nstr);

  UA_Server_writeInverseName(ns->master->master, ns->id, lt);
  return self;
} //}}}
static VALUE node_inversename(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_LocalizedText value;
  UA_LocalizedText_init(&value);
  UA_StatusCode retval = UA_Server_readInverseName(ns->master->master, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = rb_str_export_locale(rb_str_new((char *)(value.text.data), value.text.length));
  }

  UA_LocalizedText_clear(&value);
  return ret;
} //}}}
static VALUE node_abstract_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  int b = RTEST(value);

  UA_Server_writeIsAbstract(ns->master->master, ns->id, b);
  return self;
} //}}}
static VALUE node_abstract(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Boolean value;
  UA_Boolean_init(&value);
  UA_StatusCode retval = UA_Server_readIsAbstract(ns->master->master, ns->id, &value);

  VALUE ret = Qfalse;
  if (retval == UA_STATUSCODE_GOOD && value == 1)
  {
    ret = Qtrue;
  }

  UA_Boolean_clear(&value);
  return ret;
} //}}}
static VALUE node_notifier_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  int val = NUM2INT(value);

  UA_Server_writeEventNotifier(ns->master->master, ns->id, val);
  return self;
} //}}}
static VALUE node_notifier(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Byte value;
  UA_Byte_init(&value);
  UA_StatusCode retval = UA_Server_readEventNotifier(ns->master->master, ns->id, &value);

  VALUE ret = rb_to_int(-1);
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = rb_to_int(INT2NUM(value));
  }

  UA_Byte_clear(&value);
  return ret;
} //}}}
static VALUE node_mask_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  int val = NUM2INT(value);

  UA_Server_writeWriteMask(ns->master->master, ns->id, val);
  return self;
} //}}}
static VALUE node_mask(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_UInt32 value;
  UA_UInt32_init(&value);
  UA_StatusCode retval = UA_Server_readWriteMask(ns->master->master, ns->id, &value);

  VALUE ret = rb_to_int(-2);
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = rb_to_int(INT2NUM(value));
  }

  UA_UInt32_clear(&value);
  return ret;
} //}}}
static VALUE node_datatype_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);
  node_struct *val;
  Data_Get_Struct(value, node_struct, val);

  UA_Server_writeDataType(ns->master->master, ns->id, val->id);
  return self;
} //}}}
static VALUE node_datatype(VALUE self)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  UA_NodeId nid;
  UA_NodeId_init(&nid);
  UA_StatusCode retval = UA_Server_readDataType(ns->master->master, ns->id, &nid);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD)
  {
    UA_NodeClass nc;
    UA_NodeClass_init(&nc);
    UA_Server_readNodeClass(ns->master->master, nid, &nc);

    if (nc == UA_NODECLASS_DATATYPE)
    {
      ret = node_wrap(cDataTypeNode, node_alloc(ns->master, nid));
    }
    UA_NodeClass_clear(&nc);
  }

  return ret;
} //}}}
static VALUE node_rank_set(VALUE self, VALUE value)
{ //{{{
  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  UA_Int32 val = NUM2INT(value);

  UA_Server_writeValueRank(ns->master->master, ns->id, val);
  return self;
} //}}}
static VALUE node_rank(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Int32 value;
  UA_Int32_init(&value);
  UA_StatusCode retval = UA_Server_readValueRank(ns->master->master, ns->id, &value);

  VALUE ret = rb_to_int(-2);
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = rb_to_int(INT2NUM(value));
  }

  UA_Int32_clear(&value);
  return ret;
} //}}}
static VALUE node_dimensions_set(VALUE self, VALUE value)
{ //{{{
  // value is the dimensions array (i.e. [2, 3, 2]), dimensionsize is just the length of this array
  //Examples for arrays/matrices :

  // ["v1","v2","v3","v4"]
  // - arraylength:   4
  // - dimensionsize: 1
  // - dimensions:   [4] (dimensions is an array with all the single sizes in each dimension)

  // [ "v11", "v12", "v13",
  //   "v21", "v22", "v23",
  //   "v31", "v32", "v33" ]
  // - arraylength:   9 (all values are in one array)
  // - dimensionsize: 2 (has two dimensions)
  // - dimensions:   [3, 3] (first dimension has length 3, second also length 3)

  // [ "v11", "v12",
  //   "v21", "v22",
  //   "v31", "v32" ]
  // - arraylength:   6 (all values are in one array)
  // - dimensionsize: 2 (has two dimensions)
  // - dimensions:   [3, 2] (first dimension has length 3, second length 2)

  // can also have 3 dimensions [3, 2, 2]
  // [ "v111", "v121",
  //   "v211", "v221",
  //   "v311", "v321"

  //   "v112", "v122",
  //   "v212", "v222",
  //   "v312", "v322" ]
  // - arraylength:   12
  // - dimensionsize: 3
  // - dimensions:   [3, 2, 2] (first dimension has length 3, second  and third length 2)
  /* 
  UA_Variant v;
  UA_UInt32 dim[2] = {3, 3}; // 3x3
  UA_Variant_setArrayCopy(&v, dim, dim.length, &UA_TYPES[UA_TYPES_UINT32]);
  UA_Server_writeArrayDimensions(ns->master->master, ns->id, v);
  UA_Variant_clear(&v);
  */
  /*
  UA_Variant v;
  UA_Double arr[9] = {1.0, 2.0, 3.0,
                    4.0, 5.0, 6.0,
                    7.0, 8.0, 9.0};
  UA_Variant_setArrayCopy(&v, arr, 9, &UA_TYPES[UA_TYPES_DOUBLE]);
  v.arrayDimensions = (UA_UInt32 *)UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]);
  v.arrayDimensionsSize = 2;
  v.arrayDimensions[0] = 3;
  v.arrayDimensions[1] = 3;
  UA_Server_writeValue(ns->master->master, ns->id, v);
  UA_Variant_clear(&v);
   */

  node_struct *ns;
  Data_Get_Struct(self, node_struct, ns);

  int size = RARRAY_LEN(value);
  UA_UInt32 *val = (UA_UInt32 *)malloc(size * sizeof(UA_UInt32));
  for (long i = 0; i < size; i++)
  {
    val[i] = NUM2UINT(rb_ary_entry(value, i));
  }
  UA_Variant v;
  UA_Variant_setArrayCopy(&v, val, size, &UA_TYPES[UA_TYPES_UINT32]);
  UA_Server_writeValueRank(ns->master->master, ns->id, size);
  UA_Server_writeArrayDimensions(ns->master->master, ns->id, v);
  UA_Variant_clear(&v);
  return self;
} //}}}
static VALUE node_dimensions(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Variant dim;
  UA_Variant_init(&dim);
  UA_StatusCode retval = UA_Server_readArrayDimensions(ns->master->master, ns->id, &dim);

  VALUE ret = rb_ary_new();
  if (retval == UA_STATUSCODE_GOOD)
  {
    for (long i = 0; i < dim.arrayLength; i++)
    {
      rb_ary_push(ret, INT2NUM(((UA_UInt32 *)dim.data)[i]));
    }
  }
  UA_Variant_clear(&dim);
  return ret;
} //}}}
static VALUE node_symmetric(VALUE self)
{ //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Boolean value;
  UA_Boolean_init(&value);
  UA_StatusCode retval = UA_Server_readSymmetric(ns->master->master, ns->id, &value);

  VALUE ret = Qfalse;
  if (retval == UA_STATUSCODE_GOOD && value == 1)
  {
    ret = Qtrue;
  }

  UA_Boolean_clear(&value);
  return ret;
} //}}}

/* -- */
static void server_free(server_struct *pss)
{ //{{{
  if (pss != NULL)
  {
    UA_Server_delete(pss->master);
    free(pss);
  }
} //}}}
static VALUE server_alloc(VALUE self)
{ //{{{
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->master = UA_Server_new();
  pss->config = UA_Server_getConfig(pss->master);
  pss->default_ns = 1;
  pss->debug = true;
  pss->methods = rb_hash_new();

  UA_ServerConfig_setDefault(pss->config);

  return Data_Wrap_Struct(self, NULL, server_free, pss);
} //}}}
/* ++ */

static VALUE server_init(VALUE self)
{ //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->master);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
} //}}}
static VALUE server_run(VALUE self)
{ //{{{
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->master, false);

  return rb_float_new(timeout / 1000.0);
} //}}}
static VALUE server_add_namespace(VALUE self, VALUE name)
{ //{{{
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
static VALUE server_types(VALUE self)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cTypeTopNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE)));
} //}}}
static VALUE server_references(VALUE self)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cReferenceTopNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES)));
} //}}}
static VALUE server_objects(VALUE self)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);
  return node_wrap(cObjectNode, node_alloc(pss, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)));
} //}}}
static VALUE server_debug(VALUE self)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  return (pss->debug) ? Qtrue : Qfalse;
} //}}}
static VALUE server_debug_set(VALUE self, VALUE val)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  if (val == Qtrue)
  {
    pss->config->logger = UA_Log_Stdout_;
    pss->debug = Qtrue;
  }
  else
  {
    pss->config->logger = UA_Log_None_;
    pss->debug = Qfalse;
  }
  return self;
} //}}}
static VALUE server_namespaces(VALUE self)
{ //{{{
  server_struct *pss;
  Data_Get_Struct(self, server_struct, pss);

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Server_readValue(pss->master, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD)
  {
    ret = extract_value(value);
  }

  UA_Variant_clear(&value);
  RB_OBJ_FREEZE(ret);
  return rb_ary_entry(ret, 0);
} //}}}

void Init_server(void)
{
  mOPCUA = rb_define_module("OPCUA");
  rb_define_const(mOPCUA, "MANDATORY", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORY));
  rb_define_const(mOPCUA, "MANDATORYPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_MANDATORYPLACEHOLDER));
  rb_define_const(mOPCUA, "OPTIONAL", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONAL));
  rb_define_const(mOPCUA, "OPTIONALPLACEHOLDER", INT2NUM(UA_NS0ID_MODELLINGRULE_OPTIONALPLACEHOLDER));
  rb_define_const(mOPCUA, "BASEDATAVARIABLETYPE", INT2NUM(UA_NS0ID_BASEDATAVARIABLETYPE));
  rb_define_const(mOPCUA, "PROPERTYTYPE", INT2NUM(UA_NS0ID_PROPERTYTYPE));

  Init_types();

  cServer = rb_define_class_under(mOPCUA, "Server", rb_cObject);
  cNode = rb_define_class_under(cServer, "Node", rb_cObject);
  cObjectNode = rb_define_class_under(cServer, "ObjectNode", cNode);
  cTypeTopNode = rb_define_class_under(cServer, "TypeTopNode", cNode);
  cTypeSubNode = rb_define_class_under(cServer, "TypeSubNode", cNode);
  cVarNode = rb_define_class_under(cServer, "ObjectVarNode", cNode);
  cMethodNode = rb_define_class_under(cServer, "ObjectMethodNode", cNode);
  cReferenceTypeNode = rb_define_class_under(cServer, "ReferenceTypeNode", cNode);
  cVariableTypeNode = rb_define_class_under(cServer, "VariableTypeNode", cNode);
  cDataTypeNode = rb_define_class_under(cServer, "DataTypeNode", cNode);
  cReferenceTopNode = rb_define_class_under(cServer, "ReferenceTopNode", cNode);
  cReferenceSubNode = rb_define_class_under(cServer, "ReferenceSubNode", cNode);
  cReferenceNode = rb_define_class_under(cServer, "ObjectReferenceNode", cNode);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", server_run, 0);
  rb_define_method(cServer, "add_namespace", server_add_namespace, 1);
  rb_define_method(cServer, "types", server_types, 0);
  rb_define_method(cServer, "references", server_references, 0);
  rb_define_method(cServer, "objects", server_objects, 0);
  rb_define_method(cServer, "debug", server_debug, 0);
  rb_define_method(cServer, "debug=", server_debug_set, 1);
  rb_define_method(cServer, "namespaces", server_namespaces, 0);
  rb_define_method(cServer, "add_reference_type", server_add_reference_type, 5);
  rb_define_method(cServer, "add_data_type", server_add_data_type, 4);
  rb_define_method(cServer, "add_variable_type", server_add_variable_type, 6);
  rb_define_method(cServer, "add_variable", server_add_variable, 5);
  rb_define_method(cServer, "add_object_type", server_add_object_type, 4);
  rb_define_method(cServer, "add_object", server_add_object, 5);
  rb_define_method(cServer, "add_method", server_add_method, 4);
  // TODO:
  // server methods to add in server address space:
  // server.add_type(name, id, parent_id, ref_id, nodeclass)
  // node methods to add to node
  // server.find_nodeid(parent_id).add_type(id, ref_id, nodeclass)

  // TODO:
  // to add instances in server address space:
  // server.add(name, id, parent_id, ref_id, type_id)

  // TODO: use link or add_reference
  // server.link(nodeid, ref_id, target_id)

  // TODO: references
  // server.references(nodeid)

  // TODO: use follow to follow a specific reference
  // server.follow(nodeid, ref_id)
  rb_define_method(cServer, "get", server_get, -1);

  rb_define_method(cNode, "to_s", node_to_s, 0);
  rb_define_method(cNode, "id", node_id, 0);
  rb_define_method(cNode, "name", node_name, 0);
  rb_define_method(cNode, "description", node_description, 0);
  rb_define_method(cNode, "description=", node_description_set, 1);
  rb_define_method(cNode, "follow_reference", node_follow, 2);

  // TODO: use link or add_reference
  // node.link(ref_id, target_id)

  // TODO: references
  // node.references

  // TODO: use follow to follow a specific reference
  // node.follow(ref_id)
  rb_define_method(cNode, "exists?", node_exists, 0);

  rb_define_method(cTypeTopNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypeTopNode, "add_reference_type", node_add_reference_type, 1);
  rb_define_method(cTypeTopNode, "folder", node_type_folder, 0);
  rb_define_method(cTypeTopNode, "abstract", node_abstract, 0);
  rb_define_method(cTypeTopNode, "abstract=", node_abstract_set, 1);

  rb_define_method(cTypeSubNode, "add_object_type", node_add_object_type, 1);
  rb_define_method(cTypeSubNode, "add_variable", node_add_variable, -1);
  rb_define_method(cTypeSubNode, "add_variable_rw", node_add_variable_rw, -1);
  rb_define_method(cTypeSubNode, "add_object", node_add_object, -1);
  rb_define_method(cTypeSubNode, "add_method", node_add_method, -1);
  rb_define_method(cTypeSubNode, "abstract", node_abstract, 0);
  rb_define_method(cTypeSubNode, "abstract=", node_abstract_set, 1);

  rb_define_method(cVariableTypeNode, "abstract", node_abstract, 0);
  rb_define_method(cVariableTypeNode, "abstract=", node_abstract_set, 1);
  rb_define_method(cVariableTypeNode, "mask", node_mask, 0);
  rb_define_method(cVariableTypeNode, "mask=", node_mask_set, 1);
  rb_define_method(cVariableTypeNode, "rank", node_rank, 0);
  rb_define_method(cVariableTypeNode, "rank=", node_rank_set, 1);
  rb_define_method(cVariableTypeNode, "dimensions", node_dimensions, 0);
  rb_define_method(cVariableTypeNode, "dimensions=", node_dimensions_set, 1);
  rb_define_method(cVariableTypeNode, "datatype", node_datatype, 0);
  rb_define_method(cVariableTypeNode, "datatype=", node_datatype_set, 1);

  rb_define_method(cReferenceTypeNode, "abstract", node_abstract, 0);
  rb_define_method(cReferenceTypeNode, "abstract=", node_abstract_set, 1);
  rb_define_method(cReferenceTypeNode, "inverse", node_inversename, 0);
  rb_define_method(cReferenceTypeNode, "inverse=", node_inversename_set, 1);
  rb_define_method(cReferenceTypeNode, "symmetric", node_symmetric, 0);
  // The following will not work, as Symmetric has to be defined when creating of the node
  // see: https://github.com/open62541/open62541/blob/d871105cc2bf1c01b36018d2dd9de551c1a621d5/include/open62541/server.h#L307
  // rb_define_method(cNode, "symmetric=", node_symmetric_set, 1);
  rb_define_method(cTypeSubNode, "add_reference", node_add_reference, 2);
  rb_define_method(cTypeSubNode, "delete!", node_delete, 0);

  rb_define_method(cObjectNode, "manifest", node_manifest, 2);
  rb_define_method(cObjectNode, "find", node_find, 1);
  rb_define_method(cObjectNode, "notifier", node_notifier, 0);
  rb_define_method(cObjectNode, "notifier=", node_notifier_set, 1);
  rb_define_method(cObjectNode, "delete!", node_delete, 0);

  rb_define_method(cVarNode, "value", node_value, 0);
  rb_define_method(cVarNode, "value=", node_value_set, 1);
  rb_define_method(cVarNode, "mask", node_mask, 0);
  rb_define_method(cVarNode, "mask=", node_mask_set, 1);
  rb_define_method(cVarNode, "rank", node_rank, 0);
  rb_define_method(cVarNode, "rank=", node_rank_set, 1);
  rb_define_method(cVarNode, "dimensions", node_dimensions, 0);
  rb_define_method(cVarNode, "dimensions=", node_dimensions_set, 1);
  rb_define_method(cVarNode, "datatype", node_datatype, 0);
  rb_define_method(cVarNode, "datatype=", node_datatype_set, 1);
  rb_define_method(cVarNode, "delete!", node_delete, 0);
}
