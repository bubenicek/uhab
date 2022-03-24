PROJECT = uhab

PROJECT_DIRS += src src/repository src/automation src/automation/jscript src/config
PROJECT_DIRS += src/binding src/binding/modbus src/binding/system src/binding/dmx src/binding/vehabus src/binding/ebus
PROJECT_DIRS += src/binding/snmp src/binding/http src/binding/mining
PROJECT_DIRS += src/uiprovider src/uiprovider/httpd/cgi src/uiprovider/httpd/rest-api
PROJECT_DIRS += data/romfs

PROJECT_SOURCEFILES += main.c
PROJECT_SOURCEFILES += bus.c
PROJECT_SOURCEFILES += uhab_config.c

PROJECT_SOURCEFILES += automation.c
PROJECT_SOURCEFILES += rule.c
PROJECT_SOURCEFILES += action.c
PROJECT_SOURCEFILES += rules_config.c
PROJECT_SOURCEFILES += jscript.c
PROJECT_SOURCEFILES += jscript_generate.c
PROJECT_SOURCEFILES += jscript_timer.c
PROJECT_SOURCEFILES += jscript_items.c
PROJECT_SOURCEFILES += jscript_utils.c

PROJECT_SOURCEFILES += repository.c
PROJECT_SOURCEFILES += item.c
PROJECT_SOURCEFILES += item_state.c
PROJECT_SOURCEFILES += items_config.c

PROJECT_SOURCEFILES += uiprovider.c
PROJECT_SOURCEFILES += sitemap_config.c
PROJECT_SOURCEFILES += widget.c

PROJECT_SOURCEFILES += binding.c
PROJECT_SOURCEFILES += service_config.c
PROJECT_SOURCEFILES += modbus_binding.c
PROJECT_SOURCEFILES += system_binding.c
PROJECT_SOURCEFILES += dmx_binding.c
PROJECT_SOURCEFILES += vehabus_binding.c
PROJECT_SOURCEFILES += snmp_binding.c
PROJECT_SOURCEFILES += http_binding.c
PROJECT_SOURCEFILES += mining_binding.c
PROJECT_SOURCEFILES += mining_jsonrpc.c
PROJECT_SOURCEFILES += ebus_binding.c

PROJECT_SOURCEFILES += cgicalls.c
PROJECT_SOURCEFILES += json_output.c
PROJECT_SOURCEFILES += rest_api.c
PROJECT_SOURCEFILES += rest_api_sys.c
PROJECT_SOURCEFILES += rest_api_bindings.c
PROJECT_SOURCEFILES += rest_api_sitemap.c
PROJECT_SOURCEFILES += rest_api_item.c
PROJECT_SOURCEFILES += rest_api_rules.c
PROJECT_SOURCEFILES += rest_api_uiprovider.c
PROJECT_SOURCEFILES += rest_api_repository.c

all: $(PROJECT) makebin

### Compiler options
CFLAGS += -O2
CFLAGS += -DCFG_DEBUG=1
CFLAGS += -DV7_BUILD_PROFILE=1
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-format-truncation

### Modules
MODULES += core/fs/romfs
MODULES += core/lib/modbus 
MODULES += core/lib/javascript/v7
MODULES += core/lib/yxml
MODULES += core/lib/mxml
MODULES += core/lib/roxml

### HTTPD
include $(EMBEDX_ROOT)/core/net/httpd/Makefile.inc

### SNMP
ifeq ($(TARGET),uhab-linux-a20)
include $(EMBEDX_ROOT)/core/net/snmp/Makefile.inc
else ifeq ($(TARGET),uhab-linux)
include $(EMBEDX_ROOT)/core/net/snmp/Makefile.inc
endif

### Common sources
include $(EMBEDX_ROOT)/Makefile.inc

RELEASE_DIR=release

install:
	$(eval VERSION := $(shell cat VERSION))
	$(eval RELEASE_DIR := ../../release/$(PROJECT)-$(VERSION))
	rm -rf $(RELEASE_DIR)
	mkdir -p $(RELEASE_DIR)
	cp $(PROJECT_TARGET) $(RELEASE_DIR)/uhab
	cp data/start.sh $(RELEASE_DIR)
	cp data/backup.sh $(RELEASE_DIR)
	cp VERSION $(RELEASE_DIR)
	cp README $(RELEASE_DIR)
	cp CHANGES $(RELEASE_DIR)
	cp -r data/conf $(RELEASE_DIR)/conf
	cp -r data/conf-default $(RELEASE_DIR)
	cp -r data/icons $(RELEASE_DIR)
	cp -r data/images $(RELEASE_DIR)
	cp -r data/www $(RELEASE_DIR)
	mkdir -p $(RELEASE_DIR)/log
	( cd $(RELEASE_DIR); zip -r uhab-upgrade-$(VERSION).pkg * )
