/* -- */
VALUE mTYPES = Qnil;

/* -- */
static int value_to_array(VALUE value, UA_Variant *variant) {
  int done = 0;

  if (rb_obj_is_kind_of(RARRAY_AREF(value,0),rb_cTime)) {
    UA_DateTime tmp[RARRAY_LEN(value)];
    for (int i=0; i < RARRAY_LEN(value); i++) {
      if (TYPE(RARRAY_AREF(value,i)) == T_FALSE) {
        tmp[i] = UA_DateTime_fromUnixTime(rb_time_timeval(RARRAY_AREF(value,i)).tv_sec);
      } else {
        tmp[i] = UA_DateTime_fromUnixTime(0);
      }
    }
    UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_BOOLEAN]);
    done = 1;
  } else {
    switch (TYPE(RARRAY_AREF(value,0))) {
      case T_TRUE:
      case T_FALSE:
        {
          UA_Boolean tmp[RARRAY_LEN(value)];
          for (int i=0; i < RARRAY_LEN(value); i++) {
            if (TYPE(RARRAY_AREF(value,i)) == T_FALSE) {
              tmp[i] = false;
            } else {
              tmp[i] = true;
            }
          }
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_BOOLEAN]);
          done = 1;
          break;
        }
      case T_FLOAT:
      case T_FIXNUM:
        {
          UA_Double tmp[RARRAY_LEN(value)];
          for (int i=0; i < RARRAY_LEN(value); i++) {
            if (TYPE(RARRAY_AREF(value,i)) == T_FLOAT || TYPE(RARRAY_AREF(value,i)) == T_FIXNUM) {
              tmp[i] = NUM2DBL(RARRAY_AREF(value,i));
            } else {
              tmp[i] = 0.0;
            }
          }
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_DOUBLE]);
          done = 1;
          break;
        }
      case T_STRING:
      case T_SYMBOL:
        {
          UA_String tmp[RARRAY_LEN(value)];
          for (int i=0; i < RARRAY_LEN(value); i++) {
            if (TYPE(RARRAY_AREF(value,i)) == T_STRING || TYPE(RARRAY_AREF(value,i)) == T_SYMBOL) {
              VALUE str = rb_obj_as_string(RARRAY_AREF(value,i));
              tmp[i] = UA_STRING(StringValuePtr(str));
            } else {
              tmp[i] = UA_STRING("");
            }
          }
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_STRING]);
          done = 1;
          break;
        }
      // case T_ARRAY:
      //   {
      //     UA_Variant xxx;

      //     for (int i=0; i < RARRAY_LEN(value); i++) {
      //       for (int j=0; j < RARRAY_LEN(RARRAY_AREF(value,0)); j++) {
      //         if (j < RARRAY_LEN(RARRAY_AREF(value,i)) {
      //         } else {

      //         }
      //       }
      //     }

      //     UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value) * RARRAY_LEN(RARRAY_AREF(value,0)), &UA_TYPES[UA_TYPES_STRING]);
      //     done = 2;
      //     break;
      //   }

    }
  }
  return done;
}
static bool value_to_variant(VALUE value, UA_Variant *variant) { //{{{
  bool done = false;
  if (rb_obj_is_kind_of(value,rb_cTime)) {
    UA_DateTime tmp = UA_DateTime_fromUnixTime(rb_time_timeval(value).tv_sec);
    UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_DATETIME]);
    done = true;
  } else {
    switch (TYPE(value)) {
      case T_FALSE:
        {
          UA_Boolean tmp = false;
          UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_BOOLEAN]);
          done = true;
          break;
        }
      case T_TRUE:
        {
          UA_Boolean tmp = true;
          UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_BOOLEAN]);
          done = true;
          break;
        }
      case T_FLOAT:
      case T_FIXNUM:
        {
          UA_Double tmp = NUM2DBL(value);
          UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_DOUBLE]);
          done = true;
          break;
        }
      case T_STRING:
      case T_SYMBOL:
        {
          VALUE str = rb_obj_as_string(value);
          UA_String tmp = UA_STRING(StringValuePtr(str));
          UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_STRING]);
          done = true;
          break;
        }
      case T_ARRAY:
        {
          if (value_to_array(value,variant) == 1) {
            variant->arrayDimensions = (UA_UInt32 *)UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]);
            variant->arrayDimensionsSize = 1;
            variant->arrayDimensions[0] = RARRAY_LEN(value);
            done = true;
          }
          break;
        }
    }
  }
  return done;
} //}}}
/* ++ */

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
