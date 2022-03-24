
#include "rest_api.h"

TRACE_TAG(restapi_sitemap);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif


static const char *widget_get_label(uhab_sitemap_widget_t *widget, char *buf, int bufsize)
{
   const char *label_fmt = (widget->label != NULL) ? widget->label : (widget->item != NULL) ? widget->item->label : NULL;

   if (label_fmt == NULL)
      label_fmt = "";

   // Format label by type
   if (widget->item != NULL)
   {
      uhab_item_state_get_value_fmt(&widget->item->state, widget->item->label, buf, bufsize);
   }
   else
   {
      strlcpy(buf, label_fmt, bufsize);
   }

   return buf;
}


static int rest_output_widget(struct httpd_connection *con, uhab_sitemap_t *sitemap, uhab_sitemap_widget_t *widget)
{
   uhab_sitemap_widget_mapping_t *map;
   char label[255];

   // Get formated widget label
   widget_get_label(widget, label, sizeof(label));

   rest_output_value_str(con, "widgetId", "%d", widget->id);

   switch(widget->type)
   {
      case UHAB_SITEMAP_WIDGET_FRAME:
      {
         rest_output_value_str(con, "type", "Frame");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "frame");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_TEXT:
      {
         rest_output_value_str(con, "type", "Text");
         rest_output_value_str(con, "label", label);

         if (widget->icon != NULL)
            rest_output_value_str(con, "icon", widget->icon);
      }
      break;

      case UHAB_SITEMAP_WIDGET_SWITCH:
      {
         rest_output_value_str(con, "type", "Switch");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "switch");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_SLIDER:
      {
         rest_output_value_str(con, "type", "Slider");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "slider");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_ROLLERSHUTTER:
      {
         rest_output_value_str(con, "type", "Switch");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "rollershutter");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_COLORPICKER:
      {
         rest_output_value_str(con, "type", "Colorpicker");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "slider");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_SETPOINT:
      {
         rest_output_value_str(con, "type", "Setpoint");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "none");
         rest_output_value_str(con, "label", label);
         rest_output_value_double(con, "minValue", widget->setpoint.minvalue);
         rest_output_value_double(con, "maxValue", widget->setpoint.maxvalue);
         rest_output_value_double(con, "step", widget->setpoint.step);
      }
      break;

      case UHAB_SITEMAP_WIDGET_SELECTION:
      {
         rest_output_value_str(con, "type", "Selection");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "none");
         rest_output_value_str(con, "label", label);
      }
      break;

      case UHAB_SITEMAP_WIDGET_IMAGE:
      {
         rest_output_value_str(con, "type", "Image");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "none");
         rest_output_value_str(con, "label", label);
         rest_output_value_str(con, "url", widget->image.url);
      }
      break;

      case UHAB_SITEMAP_WIDGET_WEBVIEW:
      {
         rest_output_value_str(con, "type", "Webview");
         rest_output_value_str(con, "icon", widget->icon != NULL ? widget->icon : "none");
         rest_output_value_str(con, "label", label);
         rest_output_value_int(con, "height", widget->webview.height);
         rest_output_value_str(con, "url", widget->webview.url);
      }
      break;

      default:
         TRACE_ERROR("Not supported widget type: %d", widget->type);
         return -1;
   }

   if (widget->item != NULL)
      rest_output_item(con, widget->item, "item");

   // Mappings
   rest_output_array_begin(con, "mappings");
   for (map = list_head(widget->mappings); map != NULL; map = list_item_next(map))
   {
      rest_output_object_begin(con, NULL);
      rest_output_value_str(con, "command", map->key);
      rest_output_value_str(con, "label", map->value);
      rest_output_object_end(con);
   }
   rest_output_array_end(con);

   if (widget->type == UHAB_SITEMAP_WIDGET_FRAME)
   {
      rest_output_array_begin(con, "widgets");
      for (widget = list_head(widget->widgets); widget != NULL; widget = list_item_next(widget))
      {
         rest_output_object_begin(con, NULL);

         if (rest_output_widget(con, sitemap, widget) != 0)
            return -1;

         rest_output_object_end(con);
      }
      rest_output_array_end(con);
   }
   else
   {
      if (list_length(widget->widgets) > 0)
      {
         rest_output_object_begin(con, "linkedPage");

         rest_output_value_int(con, "id", widget->id);
         rest_output_value_str(con, "title", label);
         if (widget->icon != NULL)
            rest_output_value_str(con, "icon",  widget->icon);

         rest_output_value_str(con, "link", "%s/sitemaps/%s/%d", rest_get_local_url(con, label, sizeof(label)), sitemap->name, widget->id);
         rest_output_value_bool(con, "leaf", 0);

         rest_output_object_end(con);
      }

      rest_output_array_begin(con, "widgets");
      rest_output_array_end(con);
   }

   return 0;
}

/** Get site maps */
int rest_api_get_sitemaps(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char txt[255];
   uhab_sitemap_t *sitemap;

   rest_output_begin(con, REST_API_RESULT_OK, NULL);

   rest_output_array_begin(con, NULL);

   for (sitemap = list_head(uiprovider.sitemaps); sitemap != NULL; sitemap = list_item_next(sitemap))
   {
      if (list_length(sitemap->root->widgets) > 0)
      {
         rest_output_object_begin(con, NULL);

         rest_output_value_str(con, "name", sitemap->name);
         rest_output_value_str(con, "label", sitemap->root->label);
         rest_output_value_str(con, "link", "%s/sitemaps/%s/%d", rest_get_local_url(con, txt, sizeof(txt)), sitemap->name, sitemap->root->id);

         rest_output_object_begin(con, "homepage");
         rest_output_value_str(con, "link", "%s/sitemaps/%s/%d", rest_get_local_url(con, txt, sizeof(txt)), sitemap->name, sitemap->root->id);
         rest_output_value_bool(con, "leaf", 0);
         rest_output_array_begin(con, "widgets");
         rest_output_array_end(con);
         rest_output_object_end(con);

         rest_output_object_end(con);
      }
   }

   rest_output_array_end(con);

   rest_output_end(con);

   return 0;
}

/** Get sitemap by name */
int rest_api_get_sitemap(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char txt[255];
   uhab_sitemap_t *sitemap;

   REST_API_VERIFY_PARAMS(1);

   // Find sitemap by name
   if ((sitemap = uhab_provider_get_sitemap(&uiprovider, argv[0])) == NULL)
   {
      TRACE_ERROR("Sitemap '%s' not exists", argv[0]);
      return -1;
   }

   // Add second param with root page
   snprintf(txt, sizeof(txt), "%d", sitemap->root->id);
   argv[1] = txt;

   return rest_api_get_sitemap_widget(con, restcall, argv, 2);
}

int rest_api_get_sitemap_widget(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   int id;
   uhab_sitemap_t *sitemap;
   uhab_sitemap_widget_t *widget;
   char txt[255];

   REST_API_VERIFY_PARAMS(2);

   // Find sitemap by name
   if ((sitemap = uhab_provider_get_sitemap(&uiprovider, argv[0])) == NULL)
   {
      TRACE_ERROR("Sitemap '%s' not exists", argv[0]);
      return -1;
   }

   id = atoi(argv[1]);
   if (sitemap->root->id == id)
   {
      widget = sitemap->root;
   }
   else
   {
      // Find widget by ID
      if ((widget = uhab_sitemap_widget_find(sitemap->root, id)) == NULL)
      {
         TRACE_ERROR("Widget id: %d not exists in sitemap: %s", id, argv[0]);
         return -1;
      }
   }

   if (con->longpolling)
   {
      // Wait for any state changes or timeout
      uhab_bus_waitfor_changes(widget);
   }

   rest_output_begin(con, REST_API_RESULT_OK, NULL);
   rest_output_object_begin(con, NULL);

   rest_output_value_str(con, "id", "%d", widget->id);
   rest_output_value_str(con, "title", widget_get_label(widget, txt, sizeof(txt)));
   rest_output_value_str(con, "link", "%s/sitemaps/%s/%d", rest_get_local_url(con, txt, sizeof(txt)), sitemap->name, widget->id);

   if (widget->parent != NULL)
   {
      rest_output_object_begin(con, "parent");
      rest_output_value_int(con, "id", widget->parent->id);
      rest_output_value_str(con, "title", widget_get_label(widget->parent, txt, sizeof(txt)));
      rest_output_value_str(con, "link", "%s/sitemaps/%s/%d", rest_get_local_url(con, txt, sizeof(txt)), sitemap->name, widget->parent->id);
      rest_output_value_bool(con, "leaf", 0);
      rest_output_object_end(con);
   }

   rest_output_value_bool(con, "leaf", 0);

   rest_output_array_begin(con, "widgets");
   for (widget = list_head(widget->widgets); widget != NULL; widget = list_item_next(widget))
   {
      rest_output_object_begin(con, NULL);

      if (rest_output_widget(con, sitemap, widget) != 0)
         return -1;

      rest_output_object_end(con);
   }
   rest_output_array_end(con);

   rest_output_object_end(con);
   rest_output_end(con);

   return 0;
}

int rest_api_get_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_SITEMAP_CFG_FILENAME, argv[0]);

   if (httpd_send_file(con, path) != 0)
   {
      TRACE_ERROR("Send sitemap '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;
   }

   return REST_API_OK;
}

int rest_api_set_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];
   char path2[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_SITEMAP_CFG_FILENAME ".tmp", argv[0]);
   snprintf(path2, sizeof(path2), CFG_UHAB_SITEMAP_CFG_FILENAME, argv[0]);

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive items '%s' configuration failed", argv[0]);
      throw_exception(fail);
   }

   // TODO: validate received file

   // Rename temp file active
   VERIFY(unlink(path2) == 0);
   if (rename(path, path2) != 0)
   {
      TRACE_ERROR("Rename temp file %s", path);
      throw_exception(fail);
   }

   TRACE("Sitemap '%s' configuration saved", argv[0]);

   return REST_API_OK;

fail:
   // Remove temp file
   unlink(path);
   return REST_API_ERR;
}

int rest_api_delete_sitemap_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_SITEMAP_CFG_FILENAME, argv[0]);

   if (unlink(path) != 0)
   {
      TRACE_ERROR("Delete sitemap '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;
   }

   TRACE("Sitemap '%s' configuration deleted", argv[0]);

   return REST_API_OK;
}


int rest_api_subscribe_sitemaps_events(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   TRACE("rest_api_subscribe_sitemaps_events not implemented");
   return REST_API_OK;
}
