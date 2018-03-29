#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <open62541.h>

static UA_INLINE UA_ByteString
loadFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        int errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc((fileContents.length * sizeof(UA_Byte)));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
          UA_ByteString_deleteMembers(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}

static void cleanupClient(UA_Client* client, UA_ByteString* remoteCertificate) {
    UA_ByteString_delete(remoteCertificate); /* Dereference the memory */
    UA_Client_delete(client); /* Disconnects the client internally */
}

int main(void) {
    UA_Client* client = NULL;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_ByteString*          remoteCertificate  = NULL;
    UA_ByteString*          trustList          = NULL;
    size_t                  trustListSize      = 0;
    UA_ByteString*          revocationList     = NULL;
		size_t                  revocationListSize = 0;

		UA_EndpointDescription* endpointArray      = NULL;
		size_t endpointArraySize = 0;

		UA_ByteString certificate = loadFile("opc2mqtt.der");
		UA_ByteString privateKey = loadFile("opc2mqtt_key.der");

    client = UA_Client_new(UA_ClientConfig_default);
    remoteCertificate = UA_ByteString_new();
    retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:8004",
                                    &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        cleanupClient(client, remoteCertificate);
        return (int)retval;
    }

    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t endPointCount = 0; endPointCount < endpointArraySize; endPointCount++) {
        printf("URL of endpoint %i is %.*s\n", (int)endPointCount,
               (int)endpointArray[endPointCount].endpointUrl.length,
               endpointArray[endPointCount].endpointUrl.data);
        if(endpointArray[endPointCount].securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            UA_ByteString_copy(&endpointArray[endPointCount].serverCertificate, remoteCertificate);
    }

    if(UA_ByteString_equal(remoteCertificate, &UA_BYTESTRING_NULL)) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Server does not support Security Mode of"
                     " UA_MESSAGESECURITYMODE_SIGNANDENCRYPT");
        cleanupClient(client, remoteCertificate);
        return 1;
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client); /* Disconnects the client internally */

    /* Load the trustList. Load revocationList is not supported now */
    // if(argc > MIN_ARGS) {
    //     trustListSize = (size_t)argc-MIN_ARGS;
    //     retval = UA_ByteString_allocBuffer(trustList, trustListSize);
    //     if(retval != UA_STATUSCODE_GOOD) {
    //         cleanupClient(client, remoteCertificate);
    //         return (int)retval;
    //     }

    //     for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
    //         trustList[trustListCount] = loadFile(argv[trustListCount+3]);
    //     }
		// }

    printf("halklo2\n");
    client = UA_Client_secure_new(
			UA_ClientConfig_default,
      certificate,
      privateKey,
      remoteCertificate,
      trustList,
      trustListSize,
      revocationList,
      revocationListSize,
			UA_SecurityPolicy_Basic256Sha256
		);
    UA_ByteString_deleteMembers(&certificate);
    UA_ByteString_deleteMembers(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_deleteMembers(&trustList[deleteCount]);
    }

    printf("client %d\n",client);
    printf("halklo2a\n");

    // retval = UA_Client_connect(client, "opc.tcp://localhost:8004");
    retval = UA_Client_connect_username(client, "opc.tcp://localhost:8004","OpcUaClient","SUNRISE");
    printf("halklo3\n");

    if(retval != UA_STATUSCODE_GOOD) {
      cleanupClient(client, remoteCertificate);
      return (int)retval;
    }
    printf("halklo4\n");

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    // const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    const UA_NodeId nodeId = UA_NODEID_STRING(2, "/Channel/State/progStatus");
    printf("halklo5\n");
    retval = UA_Client_readValueAttribute(client, nodeId, &value);
    printf("halklo6\n");
    printf("Status: 0x%08" PRIx32 "\n",retval);

    if (retval == UA_STATUSCODE_GOOD) {
        printf("hallo0");
      if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n", dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
      } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
        printf("hallo1");
        UA_String raw_string = *(UA_String *) value.data;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "string is: %s\n", raw_string);
      } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
        printf("hallo2");
        UA_Int32 raw_int = *(UA_Int32 *) value.data;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "int32 is: %-9s %-6d\n", "int32", raw_int);
      } else {
        printf("hallo");
      }
    }

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */

    return UA_STATUSCODE_GOOD;

}
