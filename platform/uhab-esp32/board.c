
#include "system.h"

#define TRACE_TAG		"hal"
#if ! ENABLE_TRACE_HAL
#undef TRACE
#define TRACE(...)
#endif


int board_init(void)
{   
   int res = 0;
   esp_err_t ret;
   esp_chip_info_t chip_info;

   esp_vfs_spiffs_conf_t conf = {
      .base_path = CFG_UHAB_ROOT_FS,
      .partition_label = NULL,
      .max_files = CFG_SPIFFS_MAX_OPEN_FILES,
      .format_if_mount_failed = true
    };

   // Initialize HAL drivers
   res += trace_init();
   res += hal_time_init();
   res += hal_console_init();
   res += hal_gpio_init();

   esp_chip_info(&chip_info);
   
   TRACE("ESP32 chip with %d CPU cores, WiFi%s%s, silicon revision %d, %dMB %s flash", 
      chip_info.cores,
      (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
      (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
      chip_info.revision,
      spi_flash_get_chip_size() / (1024 * 1024),
      (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
   
   nvs_flash_init();
   
   // Initialize filesystem storage
   ret = esp_vfs_spiffs_register(&conf);
   if (ret != ESP_OK) 
   {
      if (ret == ESP_FAIL) 
      {
         TRACE_ERROR("Failed to mount or format filesystem");
      } 
      else if (ret == ESP_ERR_NOT_FOUND) 
      {   
         TRACE_ERROR("Failed to find SPIFFS partition");
      } 
      else 
      {
         TRACE_ERROR("Failed to initialize SPIFFS (%d)", ret);
      }
      
      return -1;
   }
   
   //esp_spiffs_format(NULL);

   size_t total = 0, used = 0;
   ret = esp_spiffs_info(NULL, &total, &used);
   if (ret != ESP_OK) 
   {
      TRACE_ERROR("Failed to get SPIFFS partition information");
      return -1;
   } 

   TRACE("Filesystem mounted, partition size: total: %d, used: %d", total, used);
   
   return 0;
}

/** Deinitialize board */
int board_deinit(void)
{
   // Unmount partition and disable SPIFFS
   esp_vfs_spiffs_unregister(NULL);
   TRACE("Unmount filesystem");
   return 0;
}


// TODO:
int system(const char *command)
{
   return 0;
}

/** Main wrapper */
void app_main(void)
{
   extern int main(void);
   main();
}
