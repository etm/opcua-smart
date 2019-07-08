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

bool node_get_reference(UA_Server *server, UA_NodeId parent, UA_NodeId *result) {
	UA_BrowseDescription bDes;
  UA_BrowseDescription_init(&bDes);
  bDes.nodeId = parent;
  bDes.resultMask = UA_BROWSERESULTMASK_ALL;
  UA_BrowseResult bRes = UA_Server_browse(server, 1, &bDes);

  if (bRes.referencesSize > 0) {
    UA_ReferenceDescription *ref = &(bRes.references[0]);

    UA_NodeId_copy(&ref->nodeId.nodeId,result);

		UA_QualifiedName qn;  UA_QualifiedName_init(&qn);
		UA_Server_readBrowseName(server, ref->nodeId.nodeId, &qn);

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
