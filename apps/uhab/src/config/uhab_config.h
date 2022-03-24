/**
 * \file uhab_config.h        \brief uHab runtime configuration
 */
 
#ifndef __UHAB_CONFIG_H
#define __UHAB_CONFIG_H


/** Initialize config */
int uhab_config_init(void);

/** Deinitialize config */
int uhab_config_deinit(void);

// Parse string in fromat: dmx=dmx1:3,4,5
int uhab_config_parse_params(char *str, char **key, char **value, char **argv, int *argc);




/** Get service config key/value */
int uhab_config_service_get_value(const char *service, const char *key, char *buf, int bufsize);

/** Load items */
int uhab_config_items_load(const char *path, uhab_repository_t *repo);

/** Load rules */
int uhab_config_rules_load(const char *path, uhab_automation_t *au);

/** Load sitemap */
int uhab_config_sitemap_load(const char *path, uhab_sitemap_t *sitemap);



#endif   // __UHAB_CONFIG_H
