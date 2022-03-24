
#ifndef __UHAB_SITEMAP_H
#define __UHAB_SITEMAP_H

#include "httpd.h"
#include "widget.h"

/** Sitemap definition */
typedef struct uhab_sitemap
{
   struct uhab_sitemap *next;
      
   /** Sitemap name */
   char *name;
  
   /** Root of widgets */
   uhab_sitemap_widget_t *root;
   
} uhab_sitemap_t;


#endif // __UHAB_SITEMAP_H