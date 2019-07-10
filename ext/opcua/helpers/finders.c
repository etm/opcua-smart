#include "finders.h"

UA_BrowsePathResult node_browse_path(UA_Server *server, UA_NodeId relative, UA_NodeId ref, UA_QualifiedName mqn, bool inverse) {
  UA_RelativePathElement rpe;
  UA_RelativePathElement_init(&rpe);
  rpe.referenceTypeId = ref;
  rpe.isInverse = inverse;
  rpe.includeSubtypes = false;
  rpe.targetName = mqn;

  UA_BrowsePath bp;
  UA_BrowsePath_init(&bp);
  bp.startingNode = relative;
  bp.relativePath.elementsSize = 1;
  bp.relativePath.elements = &rpe;

  return UA_Server_translateBrowsePathToNodeIds(server, &bp);
}

bool server_node_get_reference(UA_Server *server, UA_NodeId parent, UA_NodeId *result, bool inverse) {
	UA_BrowseDescription bDes;
  UA_BrowseDescription_init(&bDes);
  bDes.nodeId = parent;
  bDes.resultMask = UA_BROWSERESULTMASK_ALL;
  bDes.browseDirection = inverse ? 1 : 0;
  UA_BrowseResult bRes = UA_Server_browse(server, 2, &bDes);

  if (bRes.referencesSize > 0) {
    UA_ReferenceDescription *ref = &(bRes.references[0]);

    UA_NodeId_copy(&ref->nodeId.nodeId,result);

		// UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
		// UA_Server_readBrowseName(server, ref->nodeId.nodeId, &qn);
		// printf("NS: %d ---> NodeId %u; %-16.*s\n",
		// 			 ref->nodeId.nodeId.namespaceIndex,
		// 			 ref->nodeId.nodeId.identifier.numeric,
		// 			 (int)qn.name.length,
		// 			 qn.name.data
		// );

    UA_BrowseResult_deleteMembers(&bRes);
    UA_BrowseResult_clear(&bRes);
    return true;
  }
  return false;
}

bool client_node_get_reference_by_name(UA_Client *client, UA_NodeId parent, UA_QualifiedName name, UA_NodeId *result, bool inverse) {
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;
  bReq.nodesToBrowse[0].nodeId = parent;
  bReq.nodesToBrowse[0].browseDirection = inverse ? 1 : 0;
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

  bool success = false;
  for (int i=0; i < bResp.resultsSize && !success; i++) {
    for (int j=0; j < bResp.results[i].referencesSize && !success; j++) {
      UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);

      UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
      UA_Client_readBrowseNameAttribute(client, ref->nodeId.nodeId, &qn);

      if (UA_QualifiedName_equal(&qn,&name)) {
        UA_NodeId_copy(&ref->nodeId.nodeId,result);
        success = true;
      }
    }
  }

  UA_BrowseResponse_deleteMembers(&bResp);
  UA_BrowseResponse_clear(&bResp);
  return success;
}
bool client_node_get_reference_by_type(UA_Client *client, UA_NodeId parent, UA_NodeId type, UA_NodeId *result, bool inverse) {
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;
  bReq.nodesToBrowse[0].nodeId = parent;
  bReq.nodesToBrowse[0].browseDirection = inverse ? 1 : 0;
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

  bool success = false;
  for (int i=0; i < bResp.resultsSize && !success; i++) {
    for (int j=0; j < bResp.results[i].referencesSize && !success; j++) {
      UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
      if (UA_NodeId_equal(&ref->referenceTypeId,&type)) {
        UA_NodeId_copy(&ref->nodeId.nodeId,result);
        success = true;
      }
    }
  }

  UA_BrowseResponse_deleteMembers(&bResp);
  UA_BrowseResponse_clear(&bResp);
  return success;
}

bool client_node_list_references(UA_Client *client, UA_NodeId parent, bool inverse) {
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;
  bReq.nodesToBrowse[0].nodeId = parent;
  bReq.nodesToBrowse[0].browseDirection = inverse ? 1 : 0;
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

  for (int i=0; i < bResp.resultsSize; i++) {
    for (int j=0; j < bResp.results[i].referencesSize; j++) {
      UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);

      UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
      UA_Client_readBrowseNameAttribute(client, ref->nodeId.nodeId, &qn);
      UA_QualifiedName tqn;  UA_QualifiedName_init(&tqn);
      UA_Client_readBrowseNameAttribute(client, ref->referenceTypeId, &tqn);
      printf("ns=%d;i=%u - %-16.*s(ns=%d;i=%u) -> %d:%-16.*s\n",
             ref->nodeId.nodeId.namespaceIndex,
             ref->nodeId.nodeId.identifier.numeric,
             (int)tqn.name.length,
             tqn.name.data,
             ref->referenceTypeId.namespaceIndex,
             ref->referenceTypeId.identifier.numeric,
             qn.namespaceIndex,
             (int)qn.name.length,
             qn.name.data
      );

    }
  }
  UA_BrowseResponse_deleteMembers(&bResp);
  UA_BrowseResponse_clear(&bResp);
  return false;
}
