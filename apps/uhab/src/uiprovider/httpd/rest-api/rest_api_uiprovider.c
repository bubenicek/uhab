
#include "rest_api.h"

TRACE_TAG(restapi_uiprovider);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif

int rest_api_get_icon(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   int res;
   int fd;
   const char *state;
   int state_value;
   char path[255];
   
   REST_API_VERIFY_PARAMS(1);
   
   state = httpd_get_param_value(con, "state");

   snprintf(path, sizeof(path), CFG_UHAB_ROOT_FS "/icons/%s-%s.png", argv[0], state);
   str_tolower(path);
   
   if ((fd = open(path, O_RDONLY)) < 0)
   {
      // Select icon by state 0 - 100%
      state_value = atoi(state);
      if (state_value == 0)
         state_value = 0;
      else if(state_value <= 10)
         state_value = 10;
      else if(state_value <= 20)
         state_value = 20;
      else if(state_value <= 30)
         state_value = 30;
      else if(state_value <= 40)
         state_value = 40;
      else if(state_value <= 50)
         state_value = 50;
      else if(state_value <= 60)
         state_value = 60;
      else if(state_value <= 70)
         state_value = 70;
      else if(state_value <= 80)
         state_value = 80;
      else if(state_value <= 90)
         state_value = 90;
      else
         state_value = 100;
      
      snprintf(path, sizeof(path), CFG_UHAB_ROOT_FS "/icons/%s-%d.png", argv[0], state_value);
      if ((fd = open(path, O_RDONLY)) < 0)
      {
         // Try open image without state
         snprintf(path, sizeof(path), CFG_UHAB_ROOT_FS "/icons/%s.png", argv[0]);
         if ((fd = open(path, O_RDONLY)) < 0)
         {
            TRACE_ERROR("Can't get icon '%s'", path);
            return -1;
         }
      }
   }

   //TRACE("Get icon '%s'", path);

   // Set output filename
   strlcpy(con->filename, path, sizeof(con->filename));
   
   // Send header
   httpd_send_headers(con, HTTP_HEADER_200);
   
   while((res = read(fd, con->buffer, sizeof(con->buffer))) > 0)
   {
      httpd_send(con, con->buffer, res);
   }
   
   close(fd);
   
   return 0;
}

int rest_api_set_icon(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_ICONS_DIR "/%s", argv[0]);

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive items '%s' configuration failed", argv[0]);
      throw_exception(fail);
   }
     
   TRACE("Icon '%s' saved", argv[0]);
      
   return REST_API_OK;
   
fail:     
   return REST_API_ERR;      
}