/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541.h>

#include <stdlib.h>

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
handler_TheAnswerChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    printf("The Answer has changed!\n");
}
#endif

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse)
        return UA_STATUSCODE_GOOD;
    UA_NodeId *parent = (UA_NodeId *)handle;
    printf("%d, %d --- %d ---> NodeId %d, %d\n",
           parent->namespaceIndex, parent->identifier.numeric,
           referenceTypeId.identifier.numeric, childId.namespaceIndex,
           childId.identifier.numeric);
    return UA_STATUSCODE_GOOD;
}


int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++) {
        printf("URL of endpoint %i is %.*s\n", (int)i,
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Connect to a server */
    /* anonymous connect would be: */
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    //retval = UA_Client_connect_username(client, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    printf("%s\n","Connected to Server\n" );

    UA_Variant value;


    //Read ARRAY
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId id;
    UA_ReadValueId_init(&id);
    id.attributeId = UA_ATTRIBUTEID_VALUE;
    id.nodeId = UA_NODEID_STRING(1, "MyArray");
    request.nodesToRead = &id;
    request.nodesToReadSize = 1;

    printf("TEst1\n");
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    printf("%*\n", response);
    retval = UA_STATUSCODE_GOOD;
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        retval = response.responseHeader.serviceResult;
        printf("%s\n","1" );
      }
    else if(response.resultsSize != 1 || !response.results[0].hasValue) {
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
        printf("%s\n","2" );
      }
    else if(response.results[0].value.type != &UA_TYPES[UA_TYPES_DOUBLE]) {
        retval = UA_STATUSCODE_BADTYPEMISMATCH;
        printf("%s\n","3");
      }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_deleteMembers(&response);
        printf("%s\n","4" );
        return retval;
    }

    retval = UA_STATUSCODE_BADNOTFOUND;
    UA_Double *n = (UA_Double *)response.results[0].value.data;
    printf("%s\n\n","Test" );
    for(size_t i = 0; i < response.results[0].value.arrayLength; ++i){
      printf("%lf\n", n[i]);
      //printf("%s\n", ns[i].length);
    }

    UA_ReadResponse_deleteMembers(&response);


    //UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    //UA_ReadValueId id;
    UA_ReadValueId_init(&id);
    id.attributeId = UA_ATTRIBUTEID_VALUE;
    id.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY);
    request.nodesToRead = &id;
    request.nodesToReadSize = 1;

    response = UA_Client_Service_read(client, request);

    retval = UA_STATUSCODE_GOOD;
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        retval = response.responseHeader.serviceResult;
    else if(response.resultsSize != 1 || !response.results[0].hasValue)
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
    else if(response.results[0].value.type != &UA_TYPES[UA_TYPES_STRING])
        retval = UA_STATUSCODE_BADTYPEMISMATCH;

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_deleteMembers(&response);
        return retval;
    }

    retval = UA_STATUSCODE_BADNOTFOUND;
    UA_String *ns = (UA_String *)response.results[0].value.data;
    for(size_t i = 0; i < response.results[0].value.arrayLength; ++i){
        printf("The NS is %s \n", ns[i].data);
    }

    UA_ReadResponse_deleteMembers(&response);


    //WriteArray
    printf("\nWriting MyArray Values\n");

    UA_Variant_init(&value);
    double v = 123;
    UA_Variant_setArrayCopy(&value,&v,1,&UA_TYPES[UA_TYPES_DOUBLE]);

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = UA_NODEID_STRING(1,"MyArray");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    wValue.value.value = value;
    wValue.value.hasValue = true;
    wValue.indexRange = UA_STRING("4");     //index of array

    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = &wValue;
    wReq.nodesToWriteSize = 1;

    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);
    retval = wResp.responseHeader.serviceResult;
    UA_WriteResponse_deleteMembers(&wResp);
    printf("%s\n",retval );

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
