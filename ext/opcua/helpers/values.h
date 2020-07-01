#ifndef __OPCUA_SMART_VALUES_H__
#define __OPCUA_SMART_VALUES_H__

#include <open62541.h>
#include <ruby.h>

RUBY_EXTERN bool value_to_variant(VALUE value, UA_Variant *variant, UA_UInt32 proposal);
RUBY_EXTERN void Init_types(VALUE mOPCUA);
RUBY_EXTERN VALUE extract_value(UA_Variant value);

#endif
