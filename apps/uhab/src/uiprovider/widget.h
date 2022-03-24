
#ifndef __UHAB_WIDGET_H
#define __UHAB_WIDGET_H

/** Widget types */
typedef enum
{
   UHAB_SITEMAP_WIDGET_ROOT,
   UHAB_SITEMAP_WIDGET_FRAME,
   UHAB_SITEMAP_WIDGET_TEXT,
   UHAB_SITEMAP_WIDGET_SWITCH,
   UHAB_SITEMAP_WIDGET_SLIDER,
   UHAB_SITEMAP_WIDGET_ROLLERSHUTTER,
   UHAB_SITEMAP_WIDGET_COLORPICKER,
   UHAB_SITEMAP_WIDGET_SETPOINT,
   UHAB_SITEMAP_WIDGET_SELECTION,
   UHAB_SITEMAP_WIDGET_IMAGE,
   UHAB_SITEMAP_WIDGET_WEBVIEW,
   
} uhab_sitemap_widget_type_t;


/** Mapping item */
typedef struct uhab_sitemap_widget_mapping
{
   struct uhab_sitemap_widget_mapping *next;
   char *key;
   char *value;
   
} uhab_sitemap_widget_mapping_t;


/** Widget definition */
typedef struct uhab_sitemap_widget
{
   struct uhab_sitemap_widget *next;
   
   /** Ptr to parent widget */
   struct uhab_sitemap_widget *parent;
   
   /** Widget ID */
   int id;

   /** Widget type */
   uhab_sitemap_widget_type_t type;
   
   /** Widget lable */
   char *label;
   
   /** Icon name */
   char *icon;
   
   /** Link to widget item */
   const uhab_item_t *item;

   /** Mappings */
   LIST_STRUCT(mappings);

   /** Childs widgets list */
   LIST_STRUCT(widgets);
   
   /** Widget private data */
   union
   {
      struct
      {
         double minvalue;
         double maxvalue;
         double step;
         
      } setpoint;
      
      struct
      {
         char *url;
         
      } image;
      
      struct
      {
         int height;
         char *url;
         
      } webview;
   };
   
} uhab_sitemap_widget_t;


/** Alloc sitemap widget */
uhab_sitemap_widget_t *uhab_sitemap_widget_alloc(uhab_sitemap_widget_type_t type);

/** Free sitemap widget */
void uhab_sitemap_widget_free(uhab_sitemap_widget_t *widget);

/** Find widget by ID */
uhab_sitemap_widget_t *uhab_sitemap_widget_find(uhab_sitemap_widget_t *parent, int id);

/** Print widget */
void uhab_sitemap_widgets_print(uhab_sitemap_widget_t *parent, int nested_count);

/** Find widget by item in parent widget */
uhab_sitemap_widget_t *uhab_sitemap_find_item_widget(uhab_sitemap_widget_t *parent, uhab_item_t *item);


#endif // __UHAB_WIDGET_H