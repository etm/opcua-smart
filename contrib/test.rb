require 'ffi'

module OPCUA
  class ClientConfig < FFI::Struct
    layout :timeout, :uint32,
           :secureChannelLifeTime, :uint32,
           :logger, :pointer,
           :localConnectionConfig
           :connectionFunc

           :customDataTypesSize
           :customDataTypes

           :stateCallback
  end

  class ConnectionConfg

typedef struct {
    UA_UInt32 protocolVersion;
    UA_UInt32 sendBufferSize;
    UA_UInt32 recvBufferSize;
    UA_UInt32 maxMessageSize;
    UA_UInt32 maxChunkCount;
} UA_ConnectionConfig;


typedef void (*UA_Logger)(UA_LogLevel level, UA_LogCategory category,
                          const char *msg, va_list args);



typedef struct UA_ClientConfig {
    UA_UInt32 timeout;               /* Sync response timeout in ms */
    UA_UInt32 secureChannelLifeTime; /* Lifetime in ms (then the channel needs
                                        to be renewed) */
    UA_Logger logger;
    UA_ConnectionConfig localConnectionConfig;
    UA_ConnectClientConnection connectionFunc;

    /* Custom DataTypes */
    size_t customDataTypesSize;
    const UA_DataType *customDataTypes;

    /* Callback function */
    UA_ClientStateCallback stateCallback;
} UA_ClientConfig;


/* Create a new client */
UA_Client UA_EXPORT *
UA_Client_new(UA_ClientConfig config);


# #include <stdio.h>
# #include "open62541.h"
#
# int main(void) {
#     UA_Client *client = UA_Client_new(UA_ClientConfig_default);
#     UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
#     if(retval != UA_STATUSCODE_GOOD) {
#         UA_Client_delete(client);
#         return (int)retval;
#     }
#
#     /* Read the value attribute of the node. UA_Client_readValueAttribute is a
#      * wrapper for the raw read service available as UA_Client_Service_read. */
#     UA_Variant value; /* Variants can hold scalar values and arrays of any type */
#     UA_Variant_init(&value);
#
#     /* NodeId of the variable holding the current time */
#     const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
#     retval = UA_Client_readValueAttribute(client, nodeId, &value);
#
#     if (retval == UA_STATUSCODE_GOOD &&
#         UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
#         UA_DateTime raw_date = *(UA_DateTime *) value.data;
#         UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
#         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n",
#                     dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
#     }
#
#     /* Clean up */
#     UA_Variant_deleteMembers(&value);
#     UA_Client_delete(client); /* Disconnects the client internally */
#     return UA_STATUSCODE_GOOD;
# }
