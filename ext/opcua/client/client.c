#include "client.h"

VALUE mOPCUA = Qnil;
VALUE cClient = Qnil;

static UA_INLINE UA_ByteString loadFile(const char *const path) { //{{{
    UA_ByteString fileContents = UA_STRING_NULL;
    int errno;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
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
} //}}}

/* -- */
static void  client_free(client_struct *pss) { //{{{
  if (pss != NULL) {
    UA_Client_delete(pss->client);
    free(pss);
  }
} //}}}
static VALUE client_alloc(VALUE self) { //{{{
  client_struct *pss;
  pss = (client_struct *)malloc(sizeof(client_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA client.");

  pss->client = UA_Client_new();
  pss->config = UA_Client_getConfig(pss->client);
  pss->default_ns = 1;

	return Data_Wrap_Struct(self, NULL, client_free, pss);
} //}}}
/* ++ */ //}}}
static VALUE client_init(VALUE self,VALUE url,VALUE user,VALUE pass) { //{{{
  client_struct *pss;
  UA_StatusCode retval;

  Data_Get_Struct(self, client_struct, pss);

  VALUE str = rb_obj_as_string(url);
  if (NIL_P(str) || TYPE(str) != T_STRING)
    rb_raise(rb_eTypeError, "cannot convert url to string");
  char *nstr = (char *)StringValuePtr(str);

  UA_ByteString certificate = loadFile("cert.der");
  UA_ByteString privateKey = loadFile("cert_key.der");

  /* Load the trustList. Load revocationList is not supported now */
  size_t trustListSize = 0;
  UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

  UA_ByteString *revocationList = NULL;
  size_t revocationListSize = 0;

  UA_ClientConfig_setDefaultEncryption(pss->config, certificate, privateKey,
                                       trustList, trustListSize,
                                       revocationList, revocationListSize);

  UA_ByteString_clear(&certificate);
  UA_ByteString_clear(&privateKey);
  for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
      UA_ByteString_clear(&trustList[deleteCount]);
  }

  /* Secure client connect */
  pss->config->securityMode = UA_MESSAGESECURITYMODE_NONE; /* require encryption */

  //UA_ClientConfig_setDefault(pss->config);
  ///* Listing endpoints */
  //UA_EndpointDescription* endpointArray = NULL;
  //size_t endpointArraySize = 0;
  //retval = UA_Client_getEndpoints(pss->client, "opc.tcp://localhost:4840",
  //                                              &endpointArraySize, &endpointArray);
  //if(retval != UA_STATUSCODE_GOOD) {
  //    UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  //    UA_Client_delete(pss->client);
  //    return EXIT_FAILURE;
  //}
  //printf("%i endpoints found\n", (int)endpointArraySize);

  //for(size_t i=0;i<endpointArraySize;i++) {
  //  printf("URL of endpoint %i is %.*s\n", (int)i,
  //         (int)endpointArray[i].endpointUrl.length,
  //         endpointArray[i].endpointUrl.data);

  //  printf("\tuserIdentityTokens\n");
  //  for(size_t j=0;j<endpointArray[i].userIdentityTokensSize;++j) {
  //    printf("\t\t");
  //    switch(endpointArray[i].userIdentityTokens[j].tokenType) {
  //      case UA_USERTOKENTYPE_ANONYMOUS:
  //        printf("Anonymous");
  //        break;
  //      case UA_USERTOKENTYPE_USERNAME:
  //        printf("Username");
  //        break;
  //      case UA_USERTOKENTYPE_CERTIFICATE:
  //        printf("Certificate");
  //        break;
  //      case UA_USERTOKENTYPE_ISSUEDTOKEN:
  //        printf("Issued Token");
  //        break;
  //      default:
  //        printf("No valid token type");
  //        break;
  //    }
  //    printf("(%s)\n",endpointArray[i].userIdentityTokens[j].securityPolicyUri.data);
  //  }
  //}
  //UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  printf("xxxxxxxxxxxxxxxxxxxxxx\n");

  if (NIL_P(user) || NIL_P(pass)) {
    retval = UA_Client_connect(pss->client,nstr);
  } else {
    VALUE vustr = rb_obj_as_string(user);
    if (NIL_P(vustr) || TYPE(vustr) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert user to string");
    char *ustr = (char *)StringValuePtr(vustr);

    VALUE vpstr = rb_obj_as_string(pass);
    if (NIL_P(vpstr) || TYPE(vpstr) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert user to string");
    char *pstr = (char *)StringValuePtr(vpstr);

    retval = UA_Client_connect_username(pss->client, nstr, ustr, pstr);
  }
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Client could not be started.");

  return self;
} //}}}

void Init_client(void) {
  mOPCUA = rb_define_module("OPCUA");

  cClient = rb_define_class_under(mOPCUA, "Client", rb_cObject);

  rb_define_alloc_func(cClient, client_alloc);
  rb_define_method(cClient, "initialize", client_init, 3);
}
