/* -- */
static void Init_types() {/*{{{*/
  mTYPES = rb_define_module_under(mOPCUA,"TYPES");
  rb_define_const(mTYPES, "DATETIME",            INT2NUM(UA_TYPES_DATETIME           ));
  rb_define_const(mTYPES, "BOOLEAN",             INT2NUM(UA_TYPES_BOOLEAN            ));
  rb_define_const(mTYPES, "DOUBLE",              INT2NUM(UA_TYPES_DOUBLE             ));
  rb_define_const(mTYPES, "INT32",               INT2NUM(UA_TYPES_INT32              ));
  rb_define_const(mTYPES, "INT16",               INT2NUM(UA_TYPES_INT16              ));
  rb_define_const(mTYPES, "UINT32",              INT2NUM(UA_TYPES_UINT32             ));
  rb_define_const(mTYPES, "UINT16",              INT2NUM(UA_TYPES_UINT16             ));
  rb_define_const(mTYPES, "STRING",              INT2NUM(UA_TYPES_STRING             ));
}/*}}}*/

static void set_node_to_value(node_struct *ns, VALUE value) { //{{{
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
} //}}}

static VALUE extract_value(UA_Variant value) { //{{{
  VALUE ret = rb_ary_new2(2);
  rb_ary_store(ret,0,Qnil);
  rb_ary_store(ret,1,Qnil);
  // printf("type: %s\n",value.type->typeName);
  if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
    UA_DateTime raw = *(UA_DateTime *) value.data;
    rb_ary_store(ret,0,rb_time_new(UA_DateTime_toUnixTime(raw),0));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.DateTime")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
    UA_Boolean raw = *(UA_Boolean *) value.data;
    rb_ary_store(ret,0,raw ? Qtrue : Qfalse);
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Boolean")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
    UA_Double raw = *(UA_Double *) value.data;
    rb_ary_store(ret,0,DBL2NUM(raw));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Double")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
    UA_Int32 raw = *(UA_Int32 *) value.data;
    rb_ary_store(ret,0,INT2NUM(raw));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int32")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
    UA_Int16 raw = *(UA_Int16 *) value.data;
    rb_ary_store(ret,0,INT2NUM(raw));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int16")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
    UA_UInt32 raw = *(UA_UInt32 *) value.data;
    rb_ary_store(ret,0,UINT2NUM(raw));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt32")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
    UA_UInt16 raw = *(UA_UInt16 *) value.data;
    rb_ary_store(ret,0,UINT2NUM(raw));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt16")));
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
    UA_String raw = *(UA_String *) value.data;
    rb_ary_store(ret,0,rb_str_export_locale(rb_str_new((char *)(raw.data),raw.length)));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.String")));
  }

  return ret;
} //}}}
/* ++ */
