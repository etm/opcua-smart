#include "client.h"
#include "../../../cert/cert.h"
#include "../../../cert/cert_key.h"

VALUE mOPCUA = Qnil;
VALUE cClient = Qnil;
VALUE cNode = Qnil;

void UA_Log_None_log(void *_, UA_LogLevel level, UA_LogCategory category, const char *msg, va_list args) { }
void UA_Log_None_clear(void *logContext) { }
const UA_Logger UA_Log_None_ = {UA_Log_None_log, NULL, UA_Log_None_clear};
const UA_Logger *UA_Log_None = &UA_Log_None_;

/* -- */
static void  node_free(node_struct *ns) { //{{{
  if (ns != NULL) { free(ns); }
} //}}}
static VALUE node_alloc(VALUE klass, client_struct *client, UA_NodeId nodeid) { //{{{
  node_struct *ns;
  ns = (node_struct *)malloc(sizeof(node_struct));
  if (ns == NULL)
    rb_raise(rb_eNoMemError, "No memory left for node.");

  ns->client = client;
  ns->id  = nodeid;

	return Data_Wrap_Struct(klass, NULL, node_free, ns);
} //}}}
/* ++ */
static VALUE node_value(VALUE self) { //{{{
  node_struct *ns;

  Data_Get_Struct(self, node_struct, ns);

  UA_Variant value;
  UA_Variant_init(&value);
  UA_StatusCode retval = UA_Client_readValueAttribute(ns->client->client, ns->id, &value);

  VALUE ret = Qnil;
  if (retval == UA_STATUSCODE_GOOD) {
    if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
      UA_DateTime raw = *(UA_DateTime *) value.data;
      ret = rb_time_new(UA_DateTime_toUnixTime(raw),0);
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
      UA_Boolean raw = *(UA_Boolean *) value.data;
      ret = raw ? Qtrue : Qfalse;
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
      UA_Double raw = *(UA_Double *) value.data;
      ret = DBL2NUM(raw);
    } else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
      UA_String raw = *(UA_String *) value.data;
      ret = rb_str_new((char *)(raw.data),raw.length);
    }
  }

  UA_Variant_clear(&value);
  return ret;
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

  UA_ByteString certificate;
                certificate.data = (UA_Byte *)cert_der;
                certificate.length  = cert_der_len;
  UA_ByteString privateKey;
                privateKey.data = (UA_Byte *)cert_key_der;
                privateKey.length = cert_key_der_len;

  /* Load the trustList. Load revocationList is not supported now */
  size_t trustListSize = 0;
  UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

  UA_ByteString *revocationList = NULL;
  size_t revocationListSize = 0;

  UA_ClientConfig_setDefaultEncryption(pss->config, certificate, privateKey,
                                       trustList, trustListSize,
                                       revocationList, revocationListSize);

  for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
      UA_ByteString_clear(&trustList[deleteCount]);
  }

  pss->config->securityMode = UA_MESSAGESECURITYMODE_NONE; /* require no encryption, but tokens might be still enc'd */
  pss->config->logger = UA_Log_None_;

  //{{{
  // UA_ClientConfig_setDefault(pss->config);
  // /* Listing endpoints */
  // UA_EndpointDescription* endpointArray = NULL;
  // size_t endpointArraySize = 0;
  // retval = UA_Client_getEndpoints(pss->client, "opc.tcp://localhost:4840",
  //                                               &endpointArraySize, &endpointArray);
  // if(retval != UA_STATUSCODE_GOOD) {
  //     UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  //     UA_Client_delete(pss->client);
  //     return EXIT_FAILURE;
  // }
  // printf("%i endpoints found\n", (int)endpointArraySize);

  // for(size_t i=0;i<endpointArraySize;i++) {
  //   printf("URL of endpoint %i is %.*s\n", (int)i,
  //          (int)endpointArray[i].endpointUrl.length,
  //          endpointArray[i].endpointUrl.data);

  //   printf("\tuserIdentityTokens\n");
  //   for(size_t j=0;j<endpointArray[i].userIdentityTokensSize;++j) {
  //     printf("\t\t");
  //     switch(endpointArray[i].userIdentityTokens[j].tokenType) {
  //       case UA_USERTOKENTYPE_ANONYMOUS:
  //         printf("Anonymous");
  //         break;
  //       case UA_USERTOKENTYPE_USERNAME:
  //         printf("Username");
  //         break;
  //       case UA_USERTOKENTYPE_CERTIFICATE:
  //         printf("Certificate");
  //         break;
  //       case UA_USERTOKENTYPE_ISSUEDTOKEN:
  //         printf("Issued Token");
  //         break;
  //       default:
  //         printf("No valid token type");
  //         break;
  //     }
  //     printf("(%s)\n",endpointArray[i].userIdentityTokens[j].securityPolicyUri.data);
  //   }
  // }
  // UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
  //}}}

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
static VALUE client_find(VALUE self,VALUE ns,VALUE id) { //{{{
  client_struct *pss;
  Data_Get_Struct(self, client_struct, pss);

  if (NIL_P(ns) || TYPE(ns) != T_FIXNUM)
    rb_raise(rb_eTypeError, "ns is not a valid (numeric) namespace id");
  if (TYPE(id) == T_FIXNUM) {
    return node_alloc(cNode, pss, UA_NODEID_NUMERIC(NUM2INT(ns), NUM2INT(id)));
  } else {
    VALUE str = rb_obj_as_string(id);
    if (NIL_P(str) || TYPE(str) != T_STRING)
      rb_raise(rb_eTypeError, "cannot convert url to string");
    char *nstr = (char *)StringValuePtr(str);

    return node_alloc(cNode, pss, UA_NODEID_STRING(NUM2INT(ns), nstr));
  }
} //}}}

void Init_client(void) {
  mOPCUA = rb_define_module("OPCUA");

  cClient = rb_define_class_under(mOPCUA, "Client", rb_cObject);
  cNode   = rb_define_class_under(mOPCUA, "cNode", rb_cObject);

  rb_define_alloc_func(cClient, client_alloc);
  rb_define_method(cClient, "initialize", client_init, 3);
  rb_define_method(cClient, "initialize", client_init, 3);
  rb_define_method(cClient, "find", client_find, 2);

  rb_define_method(cNode, "value", node_value, 0);
}
