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

		UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
		UA_Server_readBrowseName(server, ref->nodeId.nodeId, &qn);

		printf("NS: %d ---> NodeId %u; %-16.*s\n",
					 ref->nodeId.nodeId.namespaceIndex,
					 ref->nodeId.nodeId.identifier.numeric,
					 (int)qn.name.length,
					 qn.name.data
		);

    UA_BrowseResult_deleteMembers(&bRes);
    UA_BrowseResult_clear(&bRes);
    return true;
  }
  return false;
}

bool client_node_get_reference(UA_Client *client, UA_NodeId parent, UA_NodeId *result, bool inverse) {
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;
  bReq.nodesToBrowse[0].nodeId = parent;
  bReq.nodesToBrowse[0].browseDirection = inverse ? 1 : 0;
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

  if (bResp.resultsSize > 0 && bResp.results[0].referencesSize > 0) {
    UA_ReferenceDescription *ref = &(bResp.results[0].references[0]);

    UA_NodeId_copy(&ref->nodeId.nodeId,result);

		UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
		UA_Client_readBrowseNameAttribute(client, ref->nodeId.nodeId, &qn);

		printf("NS: %d ---> NodeId %u; %-16.*s\n",
					 ref->nodeId.nodeId.namespaceIndex,
					 ref->nodeId.nodeId.identifier.numeric,
					 (int)qn.name.length,
					 qn.name.data
		);

    UA_BrowseResponse_deleteMembers(&bResp);
    UA_BrowseResponse_clear(&bResp);
    return true;
  }
  return false;
}
