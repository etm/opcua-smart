/* -- */
static VALUE extract_value(UA_Variant value) { //{{{
  VALUE ret = rb_ary_new2(2);
  rb_ary_store(ret,0,Qnil);
  rb_ary_store(ret,1,Qnil);
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
  } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
    UA_String raw = *(UA_String *) value.data;
    rb_ary_store(ret,0,rb_str_export_locale(rb_str_new((char *)(raw.data),raw.length)));
    rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.String")));
  }

  return ret;
} //}}}
/* ++ */
