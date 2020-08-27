#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), common)
DIRS := $(DIRS) $(filter-out $(DIRS), pciApp)
DIRS := $(DIRS) $(filter-out $(DIRS), vmeApp)
DIRS := $(DIRS) $(filter-out $(DIRS), exploreApp)
DIRS := $(DIRS) $(filter-out $(DIRS), testApp)

# 3.14.10 style directory dependencies
# previous versions will just ignore them

define DIR_template
 $(1)_DEPEND_DIRS = configure
endef
$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))

iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))

pciApp_DEPEND_DIRS += common
vmeApp_DEPEND_DIRS += common
exploreApp_DEPEND_DIRS += pciApp
testApp_DEPEND_DIRS += pciApp exploreApp

include $(TOP)/configure/RULES_TOP


