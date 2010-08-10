#---------------------
# Set up the Environment
#
TOP=..
include $(TOP)/configure/CONFIG

#---------------------
# Install include files
#
INC += mrfcsr.h
INC += devcsr.h
INC += vmedefs.h

#---------------------
# Install DBD files
#
DBD += vmecsr.dbd

#---------------------
# Build the VME Common Library
#
LIBRARY_IOC += epicsvme

epicsvme_SRCS += devcsr.c
epicsvme_SRCS += iocreg.c
epicsvme_SRCS_Linux += devlib_compat.c
epicsvme_SRCS_RTEMS += devlib_compat.c
epicsvme_LIBS += Com

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

