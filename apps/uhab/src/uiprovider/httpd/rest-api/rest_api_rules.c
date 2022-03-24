
#include "rest_api.h"

TRACE_TAG(restapi_rules);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif


int rest_api_get_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{   
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_RULES_CFG_FILENAME, argv[0]);
   
   if (httpd_send_file(con, path) != 0)
   {
      TRACE_ERROR("Send rules '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;      
   }
     
   return REST_API_OK;   
}

int rest_api_set_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];
   char path2[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_RULES_CFG_FILENAME ".tmp", argv[0]);
   snprintf(path2, sizeof(path2), CFG_UHAB_RULES_CFG_FILENAME, argv[0]);

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive items '%s' configuration failed", argv[0]);
      throw_exception(fail);
   }
   
   // TODO: validate received file

   // Rename temp file active
   VERIFY(unlink(path2) == 0);
   if (rename(path, path2) != 0)
   {
      TRACE_ERROR("Rename temp file %s", path);
      throw_exception(fail);
   }
   
   TRACE("Rules '%s' configuration saved", argv[0]);
      
   return REST_API_OK;
   
fail:     
   // Remove temp file
   unlink(path);
   return REST_API_ERR;   
}

int rest_api_delete_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{   
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_RULES_CFG_FILENAME, argv[0]);
   
   if (unlink(path) != 0)
   {
      TRACE_ERROR("Delete rules '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;      
   }
   
   TRACE("Rukes '%s' configuration deleted", argv[0]);        
     
   return REST_API_OK;   
}
