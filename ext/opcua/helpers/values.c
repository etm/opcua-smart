#include "values.h"

VALUE mTYPES;

/* -- */
static void variant_set_one_dimension(UA_Variant *variant,UA_UInt32 len) { //{{{
  variant->arrayDimensions = (UA_UInt32 *)UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]);
  variant->arrayDimensions[0] = len;
  variant->arrayDimensionsSize = 1;
} //}}}
static bool value_to_array(VALUE value, UA_Variant *variant) {/*{{{*/
  int done = false;

  if (rb_obj_is_kind_of(RARRAY_AREF(value,0),rb_cTime)) {
    UA_DateTime tmp[RARRAY_LEN(value)];
    for (int i=0; i < RARRAY_LEN(value); i++) {
      if (TYPE(RARRAY_AREF(value,i)) == T_FALSE) {
        tmp[i] = UA_DateTime_fromUnixTime(rb_time_timeval(RARRAY_AREF(value,i)).tv_sec);
      } else {
        tmp[i] = UA_DateTime_fromUnixTime(0);
      }
    }
    variant_set_one_dimension(variant,1);
    UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_BOOLEAN]);
    done = true;
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
          variant_set_one_dimension(variant,1);
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_BOOLEAN]);
          done = true;
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
          variant_set_one_dimension(variant,1);
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_DOUBLE]);
          done = true;
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
          variant_set_one_dimension(variant,1);
          UA_Variant_setArrayCopy(variant, tmp, RARRAY_LEN(value), &UA_TYPES[UA_TYPES_STRING]);
          done = true;
          break;
        }
        //////// TODO Currently only one-dimensional data structures are supported
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
        //     done = true;
        //     break;
        //   }
    }
  }
  return done;
}/*}}}*/

bool value_to_variant(VALUE value, UA_Variant *variant, UA_UInt32 proposal) { //{{{
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
        {
          UA_Double tmp = NUM2DBL(value);
          UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_DOUBLE]);
          done = true;
          break;
        }
      case T_FIXNUM:
        {
          if (proposal == UA_TYPES_SBYTE) {
            UA_SByte tmp = (UA_SByte)NUM2CHR(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_SBYTE]);
          } else if (proposal == UA_TYPES_BYTE) {
            UA_SByte tmp = (UA_Byte)NUM2CHR(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_BYTE]);
          } else if (proposal == UA_TYPES_INT16) {
            UA_Int16 tmp = NUM2INT(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_INT16]);
          } else if (proposal == UA_TYPES_UINT16) {
            UA_UInt16 tmp = NUM2INT(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_UINT16]);
          } else if (proposal == UA_TYPES_INT32) {
            UA_Int32 tmp = NUM2LONG(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_INT32]);
          } else if (proposal == UA_TYPES_UINT32) {
            UA_UInt32 tmp = NUM2ULONG(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_UINT32]);
          } else {
            UA_Int32 tmp = NUM2LONG(value);
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_INT32]);
          }
          done = true;
          break;
        }
      case T_STRING:
      case T_SYMBOL:
        {
          VALUE str = rb_obj_as_string(value);
          if (proposal == UA_TYPES_BYTESTRING) {
            UA_ByteString tmp = UA_BYTESTRING(StringValuePtr(str));
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_BYTESTRING]);
          } else if (proposal == UA_TYPES_NODEID) {
            UA_NodeId tmp = UA_NODEID(StringValuePtr(str));
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_NODEID]);
          } else {
            UA_String tmp = UA_STRING(StringValuePtr(str));
            UA_Variant_setScalarCopy(variant, &tmp, &UA_TYPES[UA_TYPES_STRING]);
          }
          done = true;
          break;
        }
      case T_ARRAY:
        {
          if (value_to_array(value,variant)) {
            done = true;
          }
          break;
        }
    }
  }
  return done;
} //}}}

void Init_types(VALUE mOPCUA) {/*{{{*/
  mTYPES = rb_define_module_under(mOPCUA,"TYPES");
  rb_define_const(mTYPES, "DATETIME",            INT2NUM(UA_TYPES_DATETIME           ));
  rb_define_const(mTYPES, "BOOLEAN",             INT2NUM(UA_TYPES_BOOLEAN            ));
  rb_define_const(mTYPES, "FLOAT",               INT2NUM(UA_TYPES_FLOAT              ));
  rb_define_const(mTYPES, "DOUBLE",              INT2NUM(UA_TYPES_DOUBLE             ));
  rb_define_const(mTYPES, "INT32",               INT2NUM(UA_TYPES_INT32              ));
  rb_define_const(mTYPES, "INT16",               INT2NUM(UA_TYPES_INT16              ));
  rb_define_const(mTYPES, "UINT32",              INT2NUM(UA_TYPES_UINT32             ));
  rb_define_const(mTYPES, "UINT16",              INT2NUM(UA_TYPES_UINT16             ));
  rb_define_const(mTYPES, "STRING",              INT2NUM(UA_TYPES_STRING             ));
  rb_define_const(mTYPES, "BYTESTRING",          INT2NUM(UA_TYPES_BYTESTRING         ));
  rb_define_const(mTYPES, "NODEID",              INT2NUM(UA_TYPES_NODEID             ));
}/*}}}*/

static VALUE UA_TYPES_DATETIME_to_value(UA_DateTime data) { //{{{
  return rb_time_new(UA_DateTime_toUnixTime(data),0);
} //}}}
static VALUE UA_TYPES_BOOLEAN_to_value(UA_Boolean data) { //{{{
  return data ? Qtrue : Qfalse;
} //}}}
static VALUE UA_TYPES_FLOAT_to_value(UA_Float data) { //{{{
  return DBL2NUM((double)data);
} //}}}
static VALUE UA_TYPES_DOUBLE_to_value(UA_Double data) { //{{{
  return DBL2NUM(data);
} //}}}
static VALUE UA_TYPES_SBYTE_to_value(UA_SByte data) { //{{{
  return CHR2FIX((signed char)data);
} //}}}
static VALUE UA_TYPES_INT32_to_value(UA_Int32 data) { //{{{
  return INT2NUM(data);
} //}}}
static VALUE UA_TYPES_INT16_to_value(UA_Int16 data) { //{{{
  return INT2NUM(data);
} //}}}
static VALUE UA_TYPES_BYTE_to_value(UA_Byte data) { //{{{
  return CHR2FIX((unsigned char)data);
} //}}}
static VALUE UA_TYPES_UINT32_to_value(UA_UInt32 data) { //{{{
  return UINT2NUM(data);
} //}}}
static VALUE UA_TYPES_UINT16_to_value(UA_UInt16 data) { //{{{
  return UINT2NUM(data);
} //}}}
static VALUE UA_TYPES_STRING_to_value(UA_String data) { //{{{
  return rb_str_export_locale(rb_str_new((char *)(data.data),data.length));
} //}}}
static VALUE UA_TYPES_BYTESTRING_to_value(UA_ByteString data) { //{{{
  return rb_str_export_locale(rb_str_new((char *)(data.data),data.length));
} //}}}

VALUE extract_value(UA_Variant value) { //{{{
  VALUE ret = rb_ary_new2(2);
  rb_ary_store(ret,0,Qnil);
  rb_ary_store(ret,1,Qnil);
  //printf("type: %s\n",value.type->typeName);
  //printf("dim: %ld\n",value.arrayDimensionsSize);
  //printf("alen: %ld\n",value.arrayLength);
  if (value.arrayDimensionsSize == 0 && value.arrayLength == 0) {
    if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
      rb_ary_store(ret,0,UA_TYPES_DATETIME_to_value(*(UA_DateTime *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.DateTime")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
      rb_ary_store(ret,0,UA_TYPES_BOOLEAN_to_value(*(UA_Boolean *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Boolean")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
      rb_ary_store(ret,0,UA_TYPES_FLOAT_to_value(*(UA_Float *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Float")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
      rb_ary_store(ret,0,UA_TYPES_DOUBLE_to_value(*(UA_Double *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Double")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
      rb_ary_store(ret,0,UA_TYPES_INT32_to_value(*(UA_Int32 *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int32")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
      rb_ary_store(ret,0,UA_TYPES_INT16_to_value(*(UA_Int16 *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int16")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
      rb_ary_store(ret,0,UA_TYPES_UINT32_to_value(*(UA_UInt32 *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt32")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
      rb_ary_store(ret,0,UA_TYPES_UINT16_to_value(*(UA_UInt16 *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt16")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_SBYTE])) {
      rb_ary_store(ret,0,UA_TYPES_SBYTE_to_value(*(UA_SByte *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.SByte")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTE])) {
      rb_ary_store(ret,0,UA_TYPES_BYTE_to_value(*(UA_Byte *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Byte")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
      rb_ary_store(ret,0,UA_TYPES_STRING_to_value(*(UA_String *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.String")));
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTESTRING])) {
      rb_ary_store(ret,0,UA_TYPES_BYTESTRING_to_value(*(UA_ByteString *)value.data));
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.String")));
    } else {
      //printf("Unknown Datatype\n");
    }
  } else if (value.arrayDimensionsSize == 1 || (value.arrayDimensionsSize == 0 && value.arrayLength > 0)) {
    if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.DateTime")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_DATETIME_to_value(((UA_DateTime *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Boolean")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_BOOLEAN_to_value(((UA_Boolean *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Float")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_FLOAT_to_value(((UA_Float *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Double")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_DOUBLE_to_value(((UA_Double *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_INT32])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int32")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_INT32_to_value(((UA_Int32 *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_INT16])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.Int16")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_INT16_to_value(((UA_Int16 *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt32")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_UINT32_to_value(((UA_UInt32 *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.UInt16")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_UINT16_to_value(((UA_UInt16 *)value.data)[i]));
      }
    } else if (UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_STRING])) {
      VALUE res = rb_ary_new();
      rb_ary_store(ret,0,res);
      rb_ary_store(ret,1,ID2SYM(rb_intern("VariantType.String")));
      for (int i=0; i < value.arrayLength; i++) {
        rb_ary_push(res,UA_TYPES_STRING_to_value(((UA_String *)value.data)[i]));
      }
    }
  }
  return ret;
} //}}}
/* ++ */
