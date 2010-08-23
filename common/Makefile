#---------------------
# Set up the Environment
#
TOP=..
include $(TOP)/configure/CONFIG

#---------------------
# Install include files
#
INC += epicsMMIO.h
INC += epicsMMIODef.h

ifeq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifneq ($(findstring $(EPICS_MODIFICATION),1 2 3 4 5 6 7 8),)
INC += epicsEndian.h
INC += osdWireConfig.h
endif
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
