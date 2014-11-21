#---------------------
# Set up the Environment
#
TOP=..
include $(TOP)/configure/CONFIG

#---------------------
# Install include files
#
ifeq ($(EPICS_VERSION)$(EPICS_REVISION)$(EPICS_MODIFICATION),3150)
# The MMIO definitions are included in EPICS Base >=3.15.1
INC += epicsMMIO.h
INC += epicsMMIODef.h
endif

ifeq ($(EPICS_VERSION)$(EPICS_REVISION),314)
INC += epicsMMIO.h
INC += epicsMMIODef.h

ifneq ($(findstring $(EPICS_MODIFICATION),1 2 3 4 5 6 7 8),)
INC += epicsEndian.h
INC += osdWireConfig.h
endif
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
