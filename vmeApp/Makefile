#---------------------
# Set up the Environment
#
TOP=..
include $(TOP)/configure/CONFIG

#---------------------
# Install include files
#
INC += devcsr.h
INC += vmedefs.h

#---------------------
# Install DBD files
#
DBD += epicsvme.dbd

#---------------------
# Build the VME Common Library
#
LIBRARY_IOC += epicsvme

epicsvme_SRCS += devcsr.c
epicsvme_SRCS += iocreg.c
epicsvme_SRCS += vmesh.c
epicsvme_SRCS_Linux += devlib_compat.c
epicsvme_SRCS_RTEMS += devlib_compat.c
epicsvme_LIBS += Com


ifeq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifneq ($(findstring $(EPICS_MODIFICATION),1 2 3 4 5 6 7 8 9),)
epicsvme_SRCS += devLibVMEOSD.c
epicsvme_SRCS += devLibVME.c
INC += devLibVME.h
endif
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

