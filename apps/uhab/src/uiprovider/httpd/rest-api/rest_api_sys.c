
#include "rest_api.h"

TRACE_TAG(restapi_sys);
#if !ENABLE_TRACE_REST_API
#include "trace_undef.h"
#endif

extern char _sdata;      
extern char _edata;     
#define DATA_SEG_SIZE   0  //(&_edata - &_sdata)

extern char _sbss;      
extern char _ebss;     
#define BSS_SEG_SIZE    0 //(&_ebss - &_sbss)

extern char _sccmram;      
extern char _eccmram;     
#define CCM_SEG_SIZE    0 //(&_eccmram - &_sccmram)


/** Get system info */
int rest_api_sys_get_info(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   rest_output_begin(con, REST_API_RESULT_OK, NULL);

   rest_output_object_begin(con, NULL);
   rest_output_object_begin(con, "sysinfo");

   rest_output_value_str(con, "status", (system_status.init_flags == SYSTEM_READY_FLAGS) ? "Running" : "Error");
   rest_output_value_int(con, "init_flags", system_status.init_flags);
   rest_output_value_str(con, "uptime", "%s", trace_uptime());
   rest_output_value_str(con, "systime", "%s", trace_systime());
   rest_output_value_str(con, "hwver", "%d.%d", CFG_HW_VERSION_MODEL, CFG_HW_VERSION_REVISION);
   rest_output_value_str(con, "fwver", "%d.%d", CFG_FW_VERSION_MAJOR, CFG_FW_VERSION_MINOR);

   rest_output_object_begin(con, "meminfo");
   rest_output_value_int(con, "free_heap_size", (int)osMemGetFreeSize());
   rest_output_value_int(con, "total_heap_size", (int)osMemGetTotalSize());
   rest_output_value_int(con, "data_size", DATA_SEG_SIZE);
   rest_output_value_int(con, "bss_size", BSS_SEG_SIZE);
   rest_output_value_int(con, "ccm_size", CCM_SEG_SIZE);
   rest_output_value_int(con, "total_used_memory", (osMemGetTotalSize() - osMemGetFreeSize()) + DATA_SEG_SIZE + CCM_SEG_SIZE);
   rest_output_object_end(con);


   rest_output_object_end(con);
   rest_output_object_end(con);

   rest_output_end(con);

   return 0;
}

int rest_api_sys_restart(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   rest_output_begin(con, REST_API_RESULT_OK, NULL);
   rest_output_end(con);
   
   uhab_system_restart();
   
   return 0;
}

int rest_api_sys_get_log(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
#if CFG_FSLOG_ENABLED      
   int fd, len;

   if ((fd = open(FSLOG_CURRENT_NAME, O_RDONLY, 0)) < 0)
   {
      TRACE_ERROR("Can't open file %s", FSLOG_CURRENT_NAME);
      return REST_API_ERR_NOTFOUND;
   }
   
   httpd_set_content_filename(con, FSLOG_CURRENT_NAME);
   httpd_send_headers(con, HTTP_HEADER_200);

   while((len = read(fd, con->buffer, sizeof(con->buffer))) > 0)
   {
      httpd_send(con, con->buffer, len);
   }

   close(fd);
#else
   httpd_send_headers(con, HTTP_HEADER_200);
#endif
  
   return 0;
}

int rest_api_sys_get_rules(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   int fd, len;

   if ((fd = open(CFG_UHAB_RULES_JSCRIPT_FILENAME, O_RDONLY, 0)) < 0)
   {
      TRACE_ERROR("Can't open file %s", CFG_UHAB_RULES_JSCRIPT_FILENAME);
      return REST_API_ERR_NOTFOUND;
   }
   
   httpd_set_content_filename(con, CFG_UHAB_RULES_JSCRIPT_FILENAME);
   httpd_send_headers(con, HTTP_HEADER_200);

   while((len = read(fd, con->buffer, sizeof(con->buffer))) > 0)
   {
      httpd_send(con, con->buffer, len);
   }

   close(fd);
  
   return 0;
}


int rest_api_sys_upgrade(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];
   char path2[255];

   strlcpy(path, CFG_UHAB_UPGRADE_FILENAME ".tmp", sizeof(path));
   strlcpy(path2, CFG_UHAB_UPGRADE_FILENAME, sizeof(path2));

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive upgrade failed");
      throw_exception(fail);
   }

   // Rename temp file active
   if (rename(path, path2) != 0)
   {
      TRACE_ERROR("Rename temp file %s", path);
      throw_exception(fail);
   }
   
   TRACE("Upgrade saved");
      
   return REST_API_OK;
   
fail:     
   // Remove temp file
   unlink(path);
   return REST_API_ERR;   
}

int rest_api_sys_backup(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   // Create backup file
   VERIFY(system("./backup.sh") != -1);
         
   if (httpd_send_file(con, CFG_UHAB_BACKUP_FILENAME) != 0)
   {
      TRACE_ERROR("Send backup file failed");
      throw_exception(fail);
   }

   unlink(CFG_UHAB_BACKUP_FILENAME);    
    
   TRACE("Backup file sent");
     
   return REST_API_OK;
   
fail:
   unlink(CFG_UHAB_BACKUP_FILENAME);  
   return REST_API_ERR_NOTFOUND; 
}

int rest_api_sys_restore(struct httpd_connection *con, const httpd_rest_call_t *restcall, const char *argv[], int argc)
{
   char path[255];
   char path2[255];

   strlcpy(path, CFG_UHAB_BACKUP_FILENAME ".tmp", sizeof(path));
   strlcpy(path2, CFG_UHAB_BACKUP_FILENAME, sizeof(path2));

   if (httpd_recv_file(con, path) != 0)
   {
      TRACE_ERROR("Receive backup failed");
      throw_exception(fail);
   }

   // Rename temp file active
   if (rename(path, path2) != 0)
   {
      TRACE_ERROR("Rename temp file %s", path);
      throw_exception(fail);
   }
   
   TRACE("Restore done");
      
   return REST_API_OK;
   
fail:     
   // Remove temp file
   unlink(path);
   return REST_API_ERR;   
}