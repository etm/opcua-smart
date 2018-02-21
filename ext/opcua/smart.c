#include "smart.h"

VALUE mOPCUA = Qnil;
VALUE cSmart = Qnil;

/* -- */
// ***********************************************************************************
// GC
// ***********************************************************************************
void smart_free(UA_Client *client) {
  if (client != NULL) {
    UA_Client_delete(client);
  }
}
/* ++ */

VALUE smart_alloc(VALUE self) {
  UA_Client *client = UA_Client_new(UA_ClientConfig_default);

	return Data_Wrap_Struct(self, NULL, smart_free, client);
}
VALUE smart_init(VALUE self, VALUE url) {
  UA_Client *client;

  Check_Type(url, T_STRING);
  Data_Get_Struct(self, UA_Client, client);

  UA_StatusCode retval = UA_Client_connect(client, StringValuePtr(url));
	if(retval != UA_STATUSCODE_GOOD) {
			UA_Client_delete(client);
			return (int)retval;
	}

  return self;
}

// VALUE smart_value_attribute(
//     UA_Variant value; /* Variants can hold scalar values and arrays of any type */
//     UA_Variant_init(&value);
//
//     /* NodeId of the variable holding the current time */
//     const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
//     retval = UA_Client_readValueAttribute(client, nodeId, &value);
//
//     if (retval == UA_STATUSCODE_GOOD &&
//         UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
//         UA_DateTime raw_date = *(UA_DateTime *) value.data;
//         UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
//         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n",
//                     dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
//     }
//
//     /* Clean up */
//     UA_Variant_deleteMembers(&value);
//     UA_Client_delete(client); /* Disconnects the client internally */

void Init_smart(void) {
  mOPCUA = rb_define_module("OPCUA");
  cSmart = rb_define_class_under(mOPCUA, "Smart", rb_cObject);

  rb_define_alloc_func(cSmart, smart_alloc);
  rb_define_method(cSmart, "initialize", smart_init, 1);
}
