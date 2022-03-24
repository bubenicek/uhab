
#ifndef __REST_API_H
#define __REST_API_H

#include "uhab.h"
#include "httpd.h"
#include "rest_api_sys.h"
#include "rest_api_bindings.h"
#include "rest_api_sitemap.h"
#include "rest_api_item.h"
#include "rest_api_rules.h"
#include "rest_api_uiprovider.h"
#include "rest_api_repository.h"

#define REST_API_V1                 CFG_HTTPD_WWW_ROOT_DIR "/rest"
#define REST_API_FS                 CFG_HTTPD_WWW_ROOT_DIR 

#define REST_API_RESULT_OK          0
#define REST_API_RESULT_CREATED     1
#define REST_API_RESULT_ERROR      -1

#define REST_API_VERIFY_PARAMS(_req_cnt) do { \
   if (argc != _req_cnt) { \
      TRACE_ERROR("Bad number of args, %d != %d", _req_cnt, argc); \
      return REST_API_ERR_FORMAT; \
   } \
} while(0)

//
// Parameters def macros
//
#define PARAMS(_par, ...) _req, { __VA_ARGS__ }
#define I(_name, ...) {REST_PRIMITIVE, _name, __VA_ARGS__}
#define S(_name, ...) {REST_STRING, _name, __VA_ARGS__}

#define REST_UNDEF      "UNDEF"


const char *rest_get_local_url(struct httpd_connection *con, char *buf, int bufsize);
const char *rest_get_local_ipaddr(char *buf, int bufsize);


#endif // __REST_API_H
