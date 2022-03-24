/**
 * \file uhab_config.c       \brief uHab runtime configuration
 */

#include "system.h"

#include "uhab.h"
#include "romfs.h"

TRACE_TAG(cfg);
#if !ENABLE_TRACE_CONFIG
#include "trace_undef.h"
#endif


/** Initialize config */
int uhab_config_init(void)
{
   int fd = -1;
   struct romfs_fsdata_file *file = NULL;
   char path[255];

   // Initialize defaults if configurations does not exists
   while((file = romfs_next_file(file)) != NULL)
   {
      snprintf(path, sizeof(path), "%s%s", CFG_UHAB_ROOT_FS, file->name);
      if ((fd = open(path, O_RDONLY)) < 0)
      {      
         if ((fd = creat(path, 0644)) < 0)
         {
            TRACE_ERROR("Can't create file %s", path);
            throw_exception(fail);
         }
         
         if (write(fd, file->data, file->len) != file->len)
         {
            TRACE_ERROR("Write ROM file: %s failed", path);
            throw_exception(fail);
         }
         
         TRACE("Create ROM file: %s", file->name);
      }
      
      close(fd);
   }
   
   return 0;
   
fail:
   if (fd != -1)
      close(fd);
      
   return -1;
}

/** Deinitialize config */
int uhab_config_deinit(void)
{
   return 0;
}

// Parse string in fromat: dmx=dmx1:3,4,5
int uhab_config_parse_params(char *str, char **key, char **value, char **argv, int *argc)
{
   char *pp;
   
   // Key
   *key = str;
   if ((pp = strchr(str, '=')) == NULL)
      throw_exception(fail_syntax);
   
   *pp++ = '\0';
   
   // value - optionaly
   *value = pp;
   if ((pp = strchr(pp, ':')) != NULL)
   {
      *pp++ = '\0';      
   }
   else
   {
      pp = *value;
      *value = NULL;
   }
   
   // Params delimited with comma
   *argc = split_line(pp, ',', argv, *argc);
     
   return 0;
   
fail_syntax:
   return -1;   
}
