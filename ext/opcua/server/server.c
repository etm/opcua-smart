#include "server.h"

VALUE mOPCUA = Qnil;
VALUE cServer = Qnil;

/* -- */
// ***********************************************************************************
// GC
// ***********************************************************************************
void server_free(server_struct *pss) {
  if (pss != NULL) {
    UA_Server_delete(pss->server);
    UA_ServerConfig_delete(pss->config);
    free(pss);
  }
}
/* ++ */

VALUE server_alloc(VALUE self) {
  server_struct *pss;
  pss = (server_struct *)malloc(sizeof(server_struct));
  if (pss == NULL)
    rb_raise(rb_eNoMemError, "No memory left for OPCUA server.");

  pss->config = UA_ServerConfig_new_default();
  pss->server = UA_Server_new(pss->config);

	return Data_Wrap_Struct(self, NULL, server_free, pss);
}
VALUE server_init(VALUE self) {
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_StatusCode retval = UA_Server_run_startup(pss->server);
  if (retval != UA_STATUSCODE_GOOD)
    rb_raise(rb_eRuntimeError, "Server could not be started.");

  return self;
}

VALUE server_run(VALUE self) {
  server_struct *pss;

  Data_Get_Struct(self, server_struct, pss);

  UA_UInt16 timeout = UA_Server_run_iterate(pss->server, false);

  return rb_float_new(timeout/1000.0);
}

void Init_server(void) {
  mOPCUA = rb_define_module("OPCUA");
  cServer = rb_define_class_under(mOPCUA, "Server", rb_cObject);

  rb_define_alloc_func(cServer, server_alloc);
  rb_define_method(cServer, "initialize", server_init, 0);
  rb_define_method(cServer, "run", (VALUE(*)(ANYARGS))server_run, 0);
}
