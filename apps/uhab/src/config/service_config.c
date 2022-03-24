/**
 * \file service_config.c        \brief Services configuration
 */
 
#include "uhab.h"

TRACE_TAG(cfg_service);
#if !ENABLE_TRACE_CONFIG
#include "trace_undef.h"
#endif


/** Get binding config key/value */
int uhab_config_service_get_value(const char *service, const char *key, char *buf, int bufsize)
{
   int fd, keylen;
   char *pp;
   char linebuf[255];
   
   keylen = strlen(key);
   
   // Open config file
   snprintf(linebuf, sizeof(linebuf), CFG_UHAB_SERVICES_CFG_FILENAME, service);
   if ((fd = open(linebuf, O_RDONLY)) < 0)
   {
      TRACE_ERROR("Can't open items cfg file %s", linebuf);
      throw_exception(fail_open);
   }
   
   while(readline(fd, linebuf, sizeof(linebuf)) > 0)
   {
      if (!strncasecmp(linebuf, key, keylen))
      {
         if ((pp = strchr(linebuf, '=')) == NULL)
            throw_exception(fail_parse);
            
         strncpy(buf, ++pp, bufsize);
         close(fd);

         return 0;
      }
   }

   close(fd);
      
   // key not found   
   return -1;

fail_parse:
   close(fd);
fail_open:
   return -1;   
}
