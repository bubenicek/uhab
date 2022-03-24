/**
 * \file sitemap_config.c        \brief Sitemap configuration
 */
 
#include "uhab.h"
#include "roxml.h"
#include "rest_api.h"

TRACE_TAG(cfg_sitemap);
#if !ENABLE_TRACE_CONFIG
#include "trace_undef.h"
#endif

#define CFG_SITEMAP_LOCALHOST_NAME        "http://localhost"
#define CFG_SITEMAP_IPADDR_LEN            64


/** Widget definition */
typedef struct
{
   char *name;
   uhab_sitemap_widget_type_t type;
   
} uhab_widget_def_t;


// Prototypes:
static int parse_widget_node(node_t *node, uhab_sitemap_widget_t *parent);

static const uhab_widget_def_t widget_def[] = 
{
   {"frame", UHAB_SITEMAP_WIDGET_FRAME},
   {"text", UHAB_SITEMAP_WIDGET_TEXT},
   {"switch", UHAB_SITEMAP_WIDGET_SWITCH},
   {"slider", UHAB_SITEMAP_WIDGET_SLIDER},  
   {"rollershutter", UHAB_SITEMAP_WIDGET_ROLLERSHUTTER},
   {"colorpicker", UHAB_SITEMAP_WIDGET_COLORPICKER},
   {"setpoint", UHAB_SITEMAP_WIDGET_SETPOINT},      
   {"selection", UHAB_SITEMAP_WIDGET_SELECTION},     
   {"image", UHAB_SITEMAP_WIDGET_IMAGE},
   {"webview", UHAB_SITEMAP_WIDGET_WEBVIEW},
   {NULL}
};


/** Load items */
int uhab_config_sitemap_load(const char *path, uhab_sitemap_t *sitemap)
{
   int fd;
   node_t *doc, *root, *node;
   char *value; 
   const char *pe, *pb;
   char txt[255];
      
   // Open config file
   if ((fd = open(path, O_RDONLY)) < 0)
   {
      TRACE_ERROR("Can't open sitemap cfg file %s", path);
      throw_exception(fail_open);
   }
   
   TRACE("Open sitemap cfg: %s",path);

   // Create root widget
   if ((sitemap->root = uhab_sitemap_widget_alloc(UHAB_SITEMAP_WIDGET_ROOT)) == NULL)
   {
      TRACE_ERROR("Alloc widget root");
      throw_exception(fail_alloc_root);
   }

   if ((doc = roxml_load_fd(fd)) == NULL)
   {
      TRACE_ERROR("Load sitemap xml failed");
      throw_exception(fail_load);
   }
   
   if ((root = roxml_get_chld(doc, "sitemap", 0)) == NULL)
   {
      TRACE_ERROR("Sitemap not found");
      throw_exception(fail_parse);
   }
   
   // Get sitemap name from filename
   if ((pb = strrchr(path, '/')) == NULL)
      pb = path;
   else
      pb++;
   
   if ((pe = strrchr(path, '.')) == NULL)
      pe = path + strlen(path);
      
   strncpy(txt, pb, pe - pb);
   txt[pe-pb] = '\0';
  
   if ((sitemap->name = os_strdup(txt)) == NULL)
      throw_exception(fail_parse);   
         
   if ((value = roxml_get_attr_value(root, "label")) != NULL)
   {
      if ((sitemap->root->label = os_strdup(value)) == NULL)
         throw_exception(fail_parse);
   }

   // All widgets
   for (node = roxml_get_chld(root, NULL, 0); node != NULL; node = roxml_get_next_sibling(node))           
   {
      if (parse_widget_node(node, sitemap->root) != 0)
      {
         TRACE_ERROR("Parse widget node: %s", roxml_get_name(node, NULL, 0));
         throw_exception(fail_parse);
      }
   }

	roxml_release(RELEASE_ALL);
	roxml_close(doc);
	close(fd);
   
   return 0;

fail_parse:
	roxml_release(RELEASE_ALL);
	roxml_close(doc);
fail_load:	
   os_free(sitemap->root);
fail_alloc_root:
	close(fd);
fail_open:   
   return -1;  
}

static int parse_widget_node(node_t *node, uhab_sitemap_widget_t *parent)
{
   uhab_sitemap_widget_t *widget = NULL;
   const uhab_widget_def_t *wdef;
   uhab_sitemap_widget_mapping_t *map = NULL;
   const char *name;
   const char *value;
   char *token, *subtoken;
   char *saveptr1, *saveptr2;

   name = roxml_get_name(node, NULL, 0);
   for (wdef = &widget_def[0]; wdef->name != NULL; wdef++)
   {
      if (!strcasecmp(wdef->name, name))
      {
         if ((widget = uhab_sitemap_widget_alloc(wdef->type)) == NULL)
         {
            TRACE_ERROR("Alloc widget %s", wdef->name);
            return -1;
         }
      }
   }
   
   if (widget == NULL)
   {
      TRACE_ERROR("Not supported widget '%s'", name);
      return -1;
   }
   
   // Set parent
   widget->parent = parent;
   
   // Label
   if ((value = roxml_get_attr_value(node, "label")) != NULL)
   {
      if ((widget->label = os_strdup(value)) == NULL)
         throw_exception(fail_parse);
   }   
   
   // Icon
   if ((value = roxml_get_attr_value(node, "icon")) != NULL)
   {
      if ((widget->icon = os_strdup(value)) == NULL)
         throw_exception(fail_parse);
   }   

   // Item
   if ((value = roxml_get_attr_value(node, "item")) != NULL)
   {
      widget->item = uhab_repository_get_item(&repository, value);
      if (widget->item == NULL)
      {
         TRACE_ERROR("Widget item '%s' not exists", value);
         throw_exception(fail_parse);
      }     
   }

   // Mappings
   if ((value = roxml_get_attr_value(node, "mappings")) != NULL)
   {
      // Tokenize mappings key=value, ... . key/value
      token = strtok_r((char *)value, ",", &saveptr1);
      while(token != NULL)
      {
         if ((map = os_malloc(sizeof(uhab_sitemap_widget_mapping_t))) == NULL)
         {
            TRACE_ERROR("Alloc mapping");
            throw_exception(fail_parse);
         }
         os_memset(map, 0, sizeof(uhab_sitemap_widget_mapping_t));
         
         subtoken = strtok_r(token, "=", &saveptr2);
         while(subtoken != NULL)
         {
            // Get key
            if ((map->key = os_strdup(subtoken)) == NULL)
            {
               TRACE_ERROR("Alloc map key");
               throw_exception(fail_parse);
            }

            // Get value
            if ((subtoken = strtok_r(NULL, "=", &saveptr2)) == NULL)
            {
               TRACE_ERROR("Not valid mapping value");
               throw_exception(fail_parse);
            }
            if ((map->value = os_strdup(subtoken)) == NULL)
            {
               TRACE_ERROR("Alloc map value");
               throw_exception(fail_parse);
            }
            
            list_add(widget->mappings, map);
            map = NULL;

            subtoken = strtok_r(NULL, "=", &saveptr2);
         }
         
         token = strtok_r(NULL, ",", &saveptr1);
      }
   }

   
   //
   // Private widget data
   //
   
   if (widget->type == UHAB_SITEMAP_WIDGET_WEBVIEW)
   {
      if ((value = roxml_get_attr_value(node, "url")) != NULL)
      {
         if ((widget->webview.url = os_strdup(value)) == NULL)
            throw_exception(fail_parse);
      } 

      widget->webview.height = 10;
      if ((value = roxml_get_attr_value(node, "height")) != NULL)
      {
         widget->webview.height = atoi(value);
      }   
   }
   else if (widget->type == UHAB_SITEMAP_WIDGET_IMAGE)
   {
      if ((value = roxml_get_attr_value(node, "url")) != NULL)
      {
         int len;
         char *purl;
         char buf[64];
         
         if ((purl = strstr(value, CFG_SITEMAP_LOCALHOST_NAME)) != NULL)
         {
            // Localhost URL (replace localhost with local IP address and port)
            purl += strlen(CFG_SITEMAP_LOCALHOST_NAME);

            // Alloc url buffer increased about IP address length
            len = strlen(purl) + CFG_SITEMAP_IPADDR_LEN;
            if ((widget->image.url = os_malloc(len)) == NULL)
               throw_exception(fail_parse);
                                             
            // Format URL string
            snprintf(widget->image.url, len, "http://%s:%d%s", rest_get_local_ipaddr(buf, sizeof(buf)), CFG_UHAB_UIPROVIDER_HTTP_PORT, purl);
         }
         else
         {
            // Any URL
            if ((widget->image.url = os_strdup(value)) == NULL)
               throw_exception(fail_parse);
         }
         
         
      } 
   }
   else if (widget->type == UHAB_SITEMAP_WIDGET_SETPOINT)
   {
      if ((value = roxml_get_attr_value(node, "min")) != NULL)
         widget->setpoint.minvalue = atof(value);

      if ((value = roxml_get_attr_value(node, "max")) != NULL)
         widget->setpoint.maxvalue = atof(value);

      if ((value = roxml_get_attr_value(node, "step")) != NULL)
         widget->setpoint.step = atof(value);
   }

   // All inner widgets
   for (node = roxml_get_chld(node, NULL, 0); node != NULL; node = roxml_get_next_sibling(node))           
   {
      if (parse_widget_node(node, widget) != 0)
      {
         TRACE_ERROR("Parse widget node: %s", roxml_get_name(node, NULL, 0));
         throw_exception(fail_parse);
      }
   }

   // Add to widget list
   list_add(parent->widgets, widget);            

   return 0;
   
fail_parse:
   if (map != NULL)
      os_free(map);
   if (widget != NULL)
      uhab_sitemap_widget_free(widget);
   return -1;   
}
