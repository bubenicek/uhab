
#ifndef __REST_API_REPOSITORY_H
#define __REST_API_REPOSITORY_H

int rest_api_get_repositories(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

#endif // __REST_API_REPOSITORY_H