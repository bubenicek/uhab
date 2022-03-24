
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rest_api.h"

TRACE_TAG(restapi_json_output);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif


int rest_output_begin(struct httpd_connection *httpcon, int result, const char *msgtext)
{
   httpcon->element_count = 0;
      
   strcpy(httpcon->filename, "output.json");
 
   if (result == REST_API_RESULT_OK)
   {
      httpd_send_headers(httpcon, HTTP_HEADER_200);
   }
   else if (result == REST_API_RESULT_CREATED)
   {
      httpd_send_headers(httpcon, HTTP_HEADER_201);
   }
   else
   {
      httpd_send_headers(httpcon, HTTP_HEADER_500);
   }
 
   return 0;
}

int rest_output_end(struct httpd_connection *httpcon)
{
   return 0;
}

int rest_output_array_begin(struct httpd_connection *httpcon, const char *name)
{
   int res = 0;

   if (httpcon->element_count > 0)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, ",");

   if (name != NULL)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": [", name);
   else
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "[");

   httpcon->element_count = 0;
   httpd_send(httpcon, httpcon->buffer, res);

   return 0;
}

int rest_output_array_end(struct httpd_connection *httpcon)
{
   int res;

   res = snprintf(httpcon->buffer, sizeof(httpcon->buffer), "]");
   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return 0;
}

int rest_output_object_begin(struct httpd_connection *httpcon, const char *name)
{
   int res = 0;

   if (httpcon->element_count > 0)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, ",");

   if (name != NULL)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": {", name);
   else
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "{");

   httpcon->element_count = 0;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}

int rest_output_object_end(struct httpd_connection *httpcon)
{
   int res;

   res = snprintf(httpcon->buffer, sizeof(httpcon->buffer), "}");
   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}

int rest_output_value_str(struct httpd_connection *httpcon, const char *name, const char *fmt, ...)
{
   int res = 0;
   va_list args;

   if (httpcon->element_count > 0)
      res += snprintf(httpcon->buffer, sizeof(httpcon->buffer), ",");

   va_start(args, fmt);
   res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": \"", name);
   res += vsnprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, fmt, args);
   res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"");
   va_end(args);

   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}

int rest_output_value_int(struct httpd_connection *httpcon, const char *name, int value)
{
   int res = 0;

   if (httpcon->element_count > 0)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, ",");

   res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": %d", name, value);

   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}

int rest_output_value_double(struct httpd_connection *httpcon, const char *name, double value)
{
   int res = 0;

   if (httpcon->element_count > 0)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, ",");

   res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": %g", name, value);

   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}

int rest_output_value_bool(struct httpd_connection *httpcon, const char *name, int value)
{
   int res = 0;

   if (httpcon->element_count > 0)
      res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, ",");

   res += snprintf(&httpcon->buffer[res], sizeof(httpcon->buffer) - res, "\"%s\": %s", name, value ? "true" : "false");

   httpcon->element_count++;
   httpd_send(httpcon, httpcon->buffer, res);

   return res;
}
