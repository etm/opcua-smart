#include <stdio.h>
#include <open62541.h>

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse)
        return UA_STATUSCODE_GOOD;
    UA_NodeId *parent = (UA_NodeId *)handle;
    printf("%d, %d --- %d ---> NodeId %d, %d\n",
           parent->namespaceIndex,
           parent->identifier.numeric,
           referenceTypeId.identifier.numeric,
           childId.namespaceIndex,
           childId.identifier.numeric
    );

    if(childId.identifierType == UA_NODEIDTYPE_NUMERIC) {
      printf("%-9d %-16d %-16.*s %-16.*s\n",
        parent->namespaceIndex,
        childId.identifier.numeric
        // (int)referenceTypeId.browseName.name.length,
        // referenceTypeId.browseName.name.data,
        // (int)referenceTypeId.displayName.text.length,
        // referenceTypeId.displayName.text.data
      );
    } else if(childId.identifierType == UA_NODEIDTYPE_STRING) {
    //     printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
    //            (int)ref->nodeId.nodeId.identifier.string.length,
    //            ref->nodeId.nodeId.identifier.string.data,
    //            (int)ref->browseName.name.length, ref->browseName.name.data,
    //            (int)ref->displayName.text.length, ref->displayName.text.data);
    }
    return UA_STATUSCODE_GOOD;

    // if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
    //     printf("%-9d %-16d %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
    //            ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
    //            ref->browseName.name.data, (int)ref->displayName.text.length,
    //            ref->displayName.text.data);
    // } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
    //     printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
    //            (int)ref->nodeId.nodeId.identifier.string.length,
    //            ref->nodeId.nodeId.identifier.string.data,
    //            (int)ref->browseName.name.length, ref->browseName.name.data,
    //            (int)ref->displayName.text.length, ref->displayName.text.data);
    // }
}

int main(void) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if (retval == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }

    /* Clean up */
    UA_Variant_deleteMembers(&value);

    printf("Browsing nodes in objects folder:\n");
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
    // bReq.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HIERARCHICALREFERENCES); /* return hierarchicals */
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    printf("%-9s %-16s %-16s %-16s %-9s %-6s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME", "DATATYPE", "VALUE");
    for (size_t i = 0; i < bResp.resultsSize; ++i) {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                printf("%-9d %-16d %-16.*s %-16.*s", ref->browseName.namespaceIndex,
                       ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                       ref->browseName.name.data, (int)ref->displayName.text.length,
                       ref->displayName.text.data);
            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                printf("%-9d %-16.*s %-16.*s %-16.*s", ref->browseName.namespaceIndex,
                       (int)ref->nodeId.nodeId.identifier.string.length,
                       ref->nodeId.nodeId.identifier.string.data,
                       (int)ref->browseName.name.length, ref->browseName.name.data,
                       (int)ref->displayName.text.length, ref->displayName.text.data);
            }

            UA_Variant value; /* Variants can hold scalar values and arrays of any type */
            UA_Variant_init(&value);

            /* NodeId of the variable holding the current time */
            const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
            retval = UA_Client_readValueAttribute(client, ref->nodeId.nodeId, &value);

            if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
              UA_DateTime raw_date = *(UA_DateTime *) value.data;
              UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
              printf(" %-9s %u-%u-%u %u:%u:%u.%03u\n", "datetime", dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
            } else if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
              UA_Int32 raw_int = *(UA_Int32 *) value.data;
              printf(" %-9s %-6d\n", "int32", raw_int);
            } else {
              printf("\n");
            }
        }
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    // /* Same thing, this time using the node iterator... */
    // UA_NodeId *parent = UA_NodeId_new();
    // *parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    // UA_Client_forEachChildNodeCall(client, *parent, nodeIter, (void *) parent);
    // UA_NodeId_delete(parent);

    UA_Client_delete(client); /* Disconnects the client internally */

    return UA_STATUSCODE_GOOD;
}
