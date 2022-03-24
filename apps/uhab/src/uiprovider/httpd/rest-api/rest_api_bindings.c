
#include "rest_api.h"

TRACE_TAG(restapi_binding);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif


// Prototypes:
static int reconfigure_network_settings(void);


/** Get system info */
int rest_api_get_bindings(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   list_t bindings;
   uhab_protocol_binding_t *b;
   
   rest_output_begin(con, REST_API_RESULT_OK, NULL);      
   rest_output_array_begin(con, NULL);

   // Get all bindings
   bindings = uhab_binding_get_all_bindings();
   for (b = list_head(bindings); b != NULL; b = list_item_next(b))
   {
      rest_output_object_begin(con, NULL);   
      
      rest_output_value_str(con, "author", "uHAB"); 
      rest_output_value_str(con, "description", ""); 
      rest_output_value_str(con, "id", b->name);
      rest_output_value_str(con, "name", b->label);      
        
      rest_output_object_end(con);
   }
   
   rest_output_array_end(con);   
   rest_output_end(con);
   
   return 0;
}

int rest_api_get_bindings_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_SERVICES_CFG_FILENAME, argv[0]);
   
   if (httpd_send_file(con, path) != 0)
   {
      TRACE_ERROR("Send binding '%s' config failed", argv[0]);
      return REST_API_ERR_NOTFOUND;      
   }
     
   return REST_API_OK;
}

int rest_api_set_bindings_config(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   const char *binding_name = argv[0];
   char path[255];
   char path2[255];

   REST_API_VERIFY_PARAMS(1);

   snprintf(path, sizeof(path), CFG_UHAB_SERVICES_CFG_FILENAME ".tmp", binding_name);
   snprintf(path2, sizeof(path2), CFG_UHAB_SERVICES_CFG_FILENAME, binding_name);

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive binding '%s' configuration failed", binding_name);
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
   
   if (!strcasecmp(binding_name, CFG_SYSTEM_BINDING_NAME))
   {
      if (reconfigure_network_settings() != 0)
      {
         TRACE_ERROR("Reconfigure network settings");
         throw_exception(fail);
      }
   }
   
   TRACE("Binding '%s' configuration saved", binding_name);
      
   return REST_API_OK;
   
fail:     
   // Remove temp file
   unlink(path);
   return REST_API_ERR;   
}

/** Create network configuration file */
static int reconfigure_network_settings(void)
{
   FILE *fw = NULL;
   int res;
   int dhcp_enabled = 1;   
   char ipaddr[40];
   char netmask[40];
   char gw[40];
   char dns[40];
   
   if ((fw = fopen(CFG_SYSTEM_NETWORK_CFG_FILENAME, "w")) == NULL)
   {
      TRACE_ERROR("Can't create network interfaces file");
      throw_exception(fail);
   }
   
   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.dhcp", ipaddr, sizeof(ipaddr)) == 0)
      dhcp_enabled = atoi(ipaddr);
   
   if (dhcp_enabled)
   {
      fprintf(fw, "# The primary network interface DHCP\n");
      fprintf(fw, "auto lo\n");
      fprintf(fw, "iface lo inet loopback\n");
      fprintf(fw, "auto eth0\n");
      fprintf(fw, "iface eth0 inet dhcp\n");
   }
   else
   {
      res = uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.ipaddr", ipaddr, sizeof(ipaddr));
      res += uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.netmask", netmask, sizeof(netmask));
      res += uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.gw", gw, sizeof(gw));
      res += uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, "network.dns", dns, sizeof(dns));
      
      if (res != 0)
      {
         TRACE_ERROR("Read static IP address configuration failed");
         throw_exception(fail);
      }

      fprintf(fw, "# The primary network interface static IP\n");
      fprintf(fw, "auto lo\n");
      fprintf(fw, "iface lo inet loopback\n");
      fprintf(fw, "auto eth0\n");
      fprintf(fw, "iface eth0 inet static\n");
      fprintf(fw, "address %s\n", ipaddr);
      fprintf(fw, "netmask %s\n", netmask);
      fprintf(fw, "gateway %s\n", gw);
      fprintf(fw, "dns-nameservers %s\n", dns);
   }
   
   fclose(fw);
   
   TRACE("Network configuration file '%s' created", CFG_SYSTEM_NETWORK_CFG_FILENAME);
   
   return 0;
   
fail:
   if (fw != NULL)
      fclose(fw);
   
   return -1;
}
