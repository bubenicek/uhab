
#include "uhab.h"
#include "rest_api.h"

TRACE_TAG(uiprovider);
#if !ENABLE_TRACE_UIPROVIDER
#include "trace_undef.h"
#endif


static void hal_net_event_handler(hal_netif_t netif, hal_netif_event_t event)
{
}

/** Initialize UI provider */
int uhab_uiprovider_init(uhab_uiprovider_t *uiprovider)
{
   DIR *d = NULL;
   struct dirent *dir;
   uhab_sitemap_t *sitemap = NULL;
   int http_port = CFG_UHAB_UIPROVIDER_HTTP_PORT;
   hal_netif_config_t netconf = {.wifi.ssid = CFG_UIPROVIDER_DEFAULT_WIFI_SSID, .wifi.passwd = CFG_UIPROVIDER_DEFAULT_WIFI_PASSWD};
   char txt[255];

   memset(uiprovider, 0, sizeof(uhab_uiprovider_t));
   LIST_STRUCT_INIT(uiprovider, sitemaps);
   uiprovider->netif = CFG_NETIF;

   //
   // Get configuration
   //
   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.netif", txt, sizeof(txt)) == 0)
   {
      if (!strcmp(txt, "eth"))
         uiprovider->netif = HAL_NETIF_ETH;
      else if (!strcmp(txt, "wifi"))
         uiprovider->netif = HAL_NETIF_WIFI;
      else
      {
         TRACE_ERROR("Not supported network interface %s", txt);
         throw_exception(fail);
      }
   }

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.wifi.ssid", txt, sizeof(txt)) == 0)
   {
      strlcpy(netconf.wifi.ssid, txt, sizeof(netconf.wifi.ssid));
   }
   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.wifi.password", txt, sizeof(txt)) == 0)
   {
      strlcpy(netconf.wifi.passwd, txt, sizeof(netconf.wifi.passwd));
   }

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "uiprovider.http_port", txt, sizeof(txt)) == 0)
   {
      http_port = atoi(txt);
   }

   // Configure network interface
   if (hal_net_configure(CFG_NETIF, &netconf, hal_net_event_handler) != 0)
   {
      TRACE_ERROR("Configure network failed");
      throw_exception(fail);
   }
   TRACE("Network configured");

   // Start http server
   if (httpd_init(&uiprovider->httpd, http_port) != 0)
   {
      TRACE_ERROR("Start httpd server");
      throw_exception(fail);
   }
   TRACE("HTTPD initialized");

   // List sitemaps files
   if ((d = opendir(CFG_UHAB_SITEMAP_CFG_DIR)) == NULL)
   {
      TRACE_ERROR("Can't open dir %s", CFG_UHAB_SITEMAP_CFG_DIR);
      throw_exception(fail);
   }

   // Load sitemaps config
   while ((dir = readdir(d)) != NULL)
   {
      if (strstr(dir->d_name, ".sitemap") != NULL)
      {
         if ((sitemap = os_malloc(sizeof(uhab_sitemap_t))) == NULL)
         {
            TRACE_ERROR("Alloc sitemap");
            throw_exception(fail);
         }
         os_memset(sitemap, 0, sizeof(uhab_sitemap_t));

         snprintf(txt, sizeof(txt), "%s/%s", CFG_UHAB_SITEMAP_CFG_DIR, dir->d_name);
         if (uhab_config_sitemap_load(txt, sitemap) != 0)
         {
            TRACE_ERROR("Load sitemap %s", txt);
            throw_exception(fail);
         }

         list_add(uiprovider->sitemaps, sitemap);
      }
   }

   closedir(d);

   TRACE("UI provider init, IP: %s", rest_get_local_ipaddr(txt, sizeof(txt)));

   return 0;

fail:
   if (sitemap != NULL)
      os_free(sitemap);

   if (d != NULL)
      closedir(d);

   return -1;
}

/** Deinitialize UI provider */
int uhab_uiprovider_deinit(uhab_uiprovider_t *uiprovider)
{
   return 0;
}

/** Get sitemap by name */
uhab_sitemap_t *uhab_provider_get_sitemap(uhab_uiprovider_t *uiprovider, const char *name)
{
   uhab_sitemap_t *sitemap;

   for (sitemap = list_head(uiprovider->sitemaps); sitemap != NULL; sitemap = list_item_next(sitemap))
   {
      if (!strcasecmp(sitemap->name, name))
         break;
   }

   return sitemap;
}
