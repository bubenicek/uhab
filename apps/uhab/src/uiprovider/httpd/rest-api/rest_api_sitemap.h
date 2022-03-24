
#ifndef __REST_API_SITEMAP_H
#define __REST_API_SITEMAP_H

int rest_api_get_sitemaps(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_get_sitemap(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_get_sitemap_widget(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_get_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_set_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_delete_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

int rest_api_subscribe_sitemaps_events(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);


#endif // __REST_API_SITEMAP_H