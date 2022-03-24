
#ifndef __REST_API_UIPROVIDER_H
#define __REST_API_UIPROVIDER_H


int rest_api_get_icon(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_set_icon(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);


#endif // __REST_API_UIPROVIDER_H