
#include "rest_api.h"

TRACE_TAG(restapi_repository);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif

int rest_api_get_repositories(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   DIR *d = NULL;
   struct dirent *dir;   
   char *pp;
   
   if ((d = opendir(CFG_UHAB_ITEMS_CFG_DIR)) == NULL)
   {
      TRACE_ERROR("Can't open dir %s", CFG_UHAB_ITEMS_CFG_DIR);
      return -1;
   }

   rest_output_begin(con, REST_API_RESULT_OK, NULL);
   rest_output_array_begin(con, NULL);

   while ((dir = readdir(d)) != NULL)
   {
      if (strstr(dir->d_name, ".items") != NULL)
      {     
         if ((pp = strrchr(dir->d_name, '.')) != NULL)
         {
            *pp = '\0';
            
            rest_output_object_begin(con, NULL);
            rest_output_value_str(con, "name", dir->d_name);
            rest_output_object_end(con);
         }
      }
   }   
      
   rest_output_array_end(con);
   rest_output_end(con);

   closedir(d);
      
   return 0;
}
