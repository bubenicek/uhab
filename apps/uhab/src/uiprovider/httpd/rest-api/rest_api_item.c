
#include "rest_api.h"

TRACE_TAG(restapi_item);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif

int rest_output_item(struct httpd_connection *con, const uhab_item_t *item, const char *objname)
{
   char txt[255];

   rest_output_object_begin(con, objname);

   rest_output_value_str(con, "name", item->name);
   if (item->label != NULL)
   {
      rest_output_value_str(con, "label", uhab_item_state_get_value_fmt(&item->state, item->label, txt, sizeof(txt)));
   }
   else
   {
      rest_output_value_str(con, "label", REST_UNDEF);;   
   }
   rest_output_value_str(con, "link", "%s/items/%s", rest_get_local_url(con, txt, sizeof(txt)), item->name);

   switch(item->type)
   {
      case UHAB_ITEM_TYPE_SWITCH:
         rest_output_value_str(con, "type", "Switch");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_DIMMER:
         rest_output_value_str(con, "type", "Dimmer");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_CONTACT:
         rest_output_value_str(con, "type", "Contact");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_NUMBER:
         rest_output_value_str(con, "type", "Number");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_STRING:
         rest_output_value_str(con, "type", "String");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_GROUP:
      {
         rest_output_value_str(con, "type", "Group");

         if (item->stereotype == UHAB_ITEM_STEREOTYPE_LIST)
         {
            int count;
            uhab_child_item_t *child_item;

            // Count index of active item
            for (count = 0, child_item = list_head(item->child_items); child_item != item->active_child_item; child_item = list_item_next(child_item), count++);
         
            rest_output_value_int(con, "state", count);
         }
         else
         {
            rest_output_value_str(con, "state", "UNDEF");
         }
      }
      break;

      case UHAB_ITEM_TYPE_COLOR:
         rest_output_value_str(con, "type", "Color");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         rest_output_value_str(con, "category", "slider");
         break;

      case UHAB_ITEM_TYPE_ROLLERSHUTTER:
         rest_output_value_str(con, "type", "Rollershutter");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_TIMER:
         rest_output_value_str(con, "type", "Number");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_SCENE:
         rest_output_value_str(con, "type", "Number");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;

      case UHAB_ITEM_TYPE_SYSTEM:
         rest_output_value_str(con, "type", "Number");
         rest_output_value_str(con, "state", uhab_item_state_get_value(&item->state, txt, sizeof(txt)));
         break;
         
      default:
         TRACE_ERROR("Not suported item type: %d", item->type);
         return -1;
   }

   // TODO:
   rest_output_array_begin(con, "tags");
   rest_output_array_end(con);

   // TODO:
   rest_output_array_begin(con, "groupNames");
   rest_output_array_end(con);

   rest_output_object_end(con);

   return 0;
}


/** Get items */
int rest_api_get_items(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   uhab_item_t *item;

   rest_output_begin(con, REST_API_RESULT_OK, NULL);
   
   rest_output_array_begin(con, NULL);
   for (item = list_head(repository.items); item != NULL; item = list_item_next(item))
   {
      rest_output_item(con, item, NULL);
   }
   rest_output_array_end(con);

   rest_output_end(con);

   return 0;
}

/** Get item by name */
int rest_api_get_item(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   uhab_item_t *item;
   
   REST_API_VERIFY_PARAMS(1);
   
   if ((item = uhab_repository_get_item(&repository, argv[0])) == NULL)
   {
      TRACE_ERROR("Item '%s' not found", argv[0]);
      return -1;
   }

   rest_output_begin(con, REST_API_RESULT_OK, NULL);
   rest_output_item(con, item, NULL);   
   rest_output_end(con);

   return 0;
}

/** Set item by name */
int rest_api_set_item(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   int res;
   uhab_item_t *item;
   uhab_item_state_t state;
   uhab_item_state_cmd_t cmd;
   
   REST_API_VERIFY_PARAMS(1);
   
   if ((item = uhab_repository_get_item(&repository, argv[0])) == NULL)
   {
      TRACE_ERROR("Item '%s' not found", argv[0]);
      return -1;
   }
   
   if (!con->content_length)
      return -1;
   
   if ((res = httpd_socket_recv(con->sd, con->buffer, sizeof(con->buffer))) < 0)
   {
      TRACE_ERROR("Recv content");
      return -1;
   }
   
   con->buffer[res] = '\0';
     
   if (uhab_item_state_str2command(con->buffer, &cmd) == 0)
   {
      // Command
      TRACE("Set item: %s command: %d", item->name, cmd);
      uhab_item_state_set_command(&state, cmd);
   }
   else if (str_is_number(con->buffer) == 0)
   {
      // Number
      TRACE("Set item: %s number: %g", item->name, atof(con->buffer));
      uhab_item_state_set_number(&state, atof(con->buffer));
   }
   else
   {
      // String
      TRACE("Set item: %s string: '%s'", item->name, con->buffer);
      uhab_item_state_set_string(&state, con->buffer);
   }
   
   return uhab_bus_send(item, &state);
}

int rest_api_get_items_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{   
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_ITEMS_CFG_FILENAME, argv[0]);
   
   if (httpd_send_file(con, path) != 0)
   {
      TRACE_ERROR("Send items '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;      
   }
     
   return REST_API_OK;
}

int rest_api_set_items_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];
   char path2[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_ITEMS_CFG_FILENAME ".tmp", argv[0]);
   snprintf(path2, sizeof(path2), CFG_UHAB_ITEMS_CFG_FILENAME, argv[0]);

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
   
   TRACE("Items '%s' configuration saved", argv[0]);
      
   return REST_API_OK;
   
fail:     
   // Remove temp file
   unlink(path);
   return REST_API_ERR;   
}

int rest_api_delete_item_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{   
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_ITEMS_CFG_FILENAME, argv[0]);
   
   if (unlink(path) != 0)
   {
      TRACE_ERROR("Delete items '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;      
   }
   
   TRACE("Items '%s' configuration deleted", argv[0]);     
     
   return REST_API_OK;   
}
