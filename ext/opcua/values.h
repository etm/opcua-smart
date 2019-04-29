/* -- */
static VALUE extract_value(UA_Variant value) { //{{{
  VALUE ret = rb_ary_new2(2);
  rb_ary_store(ret,0,Qnil);
  rb_ary_store(ret,1,Qnil);
  // printf("type: %s\n",value.type->typeName);
  if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
    UA_DateTime raw = *(UA_DateTime *) value.data;
    rb_ary_store(ret,0,rb_time_new(UA_DateTime_toUnixTime(raw),0));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Double")));
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
