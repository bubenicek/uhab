
#include "uhab.h"

TRACE_GROUP(uiprovider);

static int widget_id_pool = 0;

/** Alloc sitemap widget */
uhab_sitemap_widget_t *uhab_sitemap_widget_alloc(uhab_sitemap_widget_type_t type)
{
   uhab_sitemap_widget_t *widget;
   
   if ((widget = os_malloc(sizeof(uhab_sitemap_widget_t))) != NULL)
   {
      os_memset(widget, 0, sizeof(uhab_sitemap_widget_t));
      widget->type = type;
      widget->id = widget_id_pool++;
      LIST_STRUCT_INIT(widget, mappings);
      LIST_STRUCT_INIT(widget, widgets);               
   }

   return widget;
}

/** Free sitemap widget */
void uhab_sitemap_widget_free(uhab_sitemap_widget_t *widget)
{
   os_free(widget);
}

/** Find widget by ID */
uhab_sitemap_widget_t *uhab_sitemap_widget_find(uhab_sitemap_widget_t *parent, int id)
{
   uhab_sitemap_widget_t *widget, *widget2;
   
   for (widget = list_head(parent->widgets); widget != NULL; widget = list_item_next(widget))
   {
      if (widget->id == id)
         break;
      
      if ((widget2 = uhab_sitemap_widget_find(widget, id)) != NULL)
      {
         widget = widget2;
         break;
      }
   }
   
   return widget;
}

void uhab_sitemap_widgets_print(uhab_sitemap_widget_t *parent, int nested_count)
{
   int ix;
   uhab_sitemap_widget_t *widget;
   
   for (widget = list_head(parent->widgets); widget != NULL; widget = list_item_next(widget))
   {
      TRACE_PRINTFF("");
      for (ix = 0; ix < nested_count * 3; ix++)
      {
         TRACE_PRINTF(" ");
      }
      TRACE_PRINTF("%d - %s  %s\n", widget->type, (widget->label != NULL) ? widget->label : (widget->item != NULL) ? widget->item->label : "", widget->item != NULL ? widget->item->name : "null");
      
      uhab_sitemap_widgets_print(widget, nested_count+1);
   }
}

/** Find widget by item in parent widget */
uhab_sitemap_widget_t *uhab_sitemap_find_item_widget(uhab_sitemap_widget_t *parent, uhab_item_t *item)
{
   uhab_sitemap_widget_t *widget_nested;
   uhab_sitemap_widget_t *widget = parent;
   
   if (parent != NULL && parent->item != item)
   {      
      for (widget = list_head(parent->widgets); widget != NULL; widget = list_item_next(widget))
      {
         if (widget->item == item)
            break;
            
         if ((widget_nested = uhab_sitemap_find_item_widget(widget, item)) != NULL)
         {
            widget = widget_nested;
            break;
         }
      }
   }
   
   return widget;
}

