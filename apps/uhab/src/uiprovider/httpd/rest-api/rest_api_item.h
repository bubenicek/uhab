
#ifndef __REST_API_ITEM_H
#define __REST_API_ITEM_H

int rest_output_item(struct httpd_connection *con, const uhab_item_t *item, const char *objname);
int rest_api_get_items(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_get_item(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_set_item(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_get_items_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_set_items_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);
int rest_api_delete_item_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

#endif // __REST_API_ITEM_H