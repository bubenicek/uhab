
#ifndef __UHAB_UIPROVIDER_H

#include "sitemap.h"


typedef struct
{
   /** Used network interface */
   hal_netif_t netif;

   /** httpd server */
   httpd_t httpd;   

   /** Sitemaps */
   LIST_STRUCT(sitemaps);
   
} uhab_uiprovider_t;


/** Initialize UI provider */
int uhab_uiprovider_init(uhab_uiprovider_t *uiprovider);

/** Deinitialize UI provider */
int uhab_uiprovider_deinit(uhab_uiprovider_t *uiprovider);

/** Get sitemap by name */
uhab_sitemap_t *uhab_provider_get_sitemap(uhab_uiprovider_t *uiprovider, const char *name);


#endif // __UHAB_UIPROVIDER_H