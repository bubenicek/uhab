
#ifndef __REST_API_BINDINGS_H
#define __REST_API_BINDINGS_H

int rest_api_get_bindings(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_get_bindings_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_set_bindings_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

#endif // __REST_API_BINDINGS_H