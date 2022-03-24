
#ifndef __REST_API_RULES_H
#define __REST_API_RULES_H

int rest_api_get_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_set_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_delete_rules_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

#endif // __REST_API_RULES_H