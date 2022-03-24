
#include "rest_api.h"

TRACE_TAG(restapi);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif


typedef struct
{
   const char *name;

} rest_api_link_t;


// Prototypes:
static int rest_api_get_root(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc);

static const rest_api_link_t rest_api_links[] = 
{
   {"bindings"},
   {"items"},
   {"sitemaps"},
   {NULL}
};



/** REST calls definitions */
const httpd_rest_call_t httpd_restcalls[] =
{
//  Version      Name,                                                        GET,     UPDATE(PUT),      INSERT(POST),     DELETE,  Object definition
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   {REST_API_V1 "/system/info",                                               rest_api_sys_get_info},
   {REST_API_V1 "/system/restart",                                            NULL, rest_api_sys_restart},
   {REST_API_V1 "/system/upgrade",                                            NULL, NULL, rest_api_sys_upgrade},
   {REST_API_V1 "/system/backup",                                             rest_api_sys_backup},
   {REST_API_V1 "/system/restore",                                            NULL, NULL, rest_api_sys_restore},
   {REST_API_V1 "/system/log",                                                rest_api_sys_get_log},
   {REST_API_V1 "/system/rules",                                              rest_api_sys_get_rules},

   {REST_API_V1 "",                                                           rest_api_get_root},
   {REST_API_V1 "/",                                                          rest_api_get_root},

   {REST_API_V1 "/bindings",                                                  rest_api_get_bindings},
   {REST_API_V1 "/bindings/config/{name}",                                    rest_api_get_bindings_config, rest_api_set_bindings_config},

   {REST_API_V1 "/sitemaps",                                                  rest_api_get_sitemaps},
   {REST_API_V1 "/sitemaps/config/{reponame}",                                rest_api_get_sitemap_config, rest_api_set_sitemap_config, NULL, rest_api_delete_sitemap_config},
   {REST_API_V1 "/sitemaps/events/subscribe",                                 NULL, NULL, rest_api_subscribe_sitemaps_events},
   {REST_API_V1 "/sitemaps/{name}",                                           rest_api_get_sitemap},
   {REST_API_V1 "/sitemaps/{name}/{widget_id}",                               rest_api_get_sitemap_widget},

   {REST_API_V1 "/items",                                                     rest_api_get_items},
   {REST_API_V1 "/items/{name}",                                              rest_api_get_item, NULL, rest_api_set_item},
   {REST_API_V1 "/items/config/{reponame}",                                   rest_api_get_items_config, rest_api_set_items_config, NULL, rest_api_delete_item_config},

   {REST_API_V1 "/icon/{name}",                                               rest_api_get_icon, NULL, rest_api_set_icon},
   {REST_API_FS "/icon/{name}",                                               rest_api_get_icon},

   {REST_API_V1 "/rules/config/{reponame}",                                   rest_api_get_rules_config, rest_api_set_rules_config, NULL, rest_api_delete_rules_config},

   {REST_API_V1 "/repositories",                                              rest_api_get_repositories},

   {NULL}
};

const char *rest_get_local_url(struct httpd_connection *con, char *buf, int bufsize)
{
   struct sockaddr_in ipaddr;
   socklen_t addrlen = sizeof(ipaddr);

   if (getsockname(con->sd, (struct sockaddr * )&ipaddr, &addrlen) != 0)
   {
      TRACE("Get Local IP address failed - %s", strerror(errno));
      snprintf(buf, bufsize, "%s", strerror(errno));
   }
   else
   {
      snprintf(buf, bufsize, "http://%s:%d/rest", inet_ntoa(ipaddr.sin_addr), CFG_UHAB_UIPROVIDER_HTTP_PORT);      
   }
   
   return buf;  
}


const char *rest_get_local_ipaddr(char *buf, int bufsize)
{  
   return hal_net_get_local_ipaddr(uiprovider.netif, buf, bufsize);
}

static int rest_api_get_root(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   const rest_api_link_t *link;
   char url[255];

   rest_get_local_url(con, url, sizeof(url));
   
   rest_output_begin(con, REST_API_RESULT_OK, NULL);

   rest_output_object_begin(con, NULL);
   rest_output_value_str(con, "version", "%d", 1);

   rest_output_array_begin(con, "links");
   for (link = &rest_api_links[0]; link->name != NULL; link++)
   {
      rest_output_object_begin(con, NULL);  
      rest_output_value_str(con, "type", "%s", link->name);
      rest_output_value_str(con, "url", "%s/%s", url, link->name);
      rest_output_object_end(con);
   }
   rest_output_array_end(con);

   rest_output_object_end(con);

   rest_output_end(con);
   
   return 0;
}
