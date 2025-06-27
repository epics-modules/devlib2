TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -I$(TOP)/common

#=============================
# Build the IOC application

LIBRARY_IOC = epicspci

SHRLIB_VERSION ?= $(DEVLIB2_MAJOR_VERSION).$(DEVLIB2_MINOR_VERSION)

USR_INCLUDES += -I../os/$(OS_CLASS)

DBD += epicspci.dbd

INC += devLibPCI.h
INC += devLibPCIImpl.h
INC += devLibPCIOSD.h

epicspci_SRCS += devLibPCI.c
epicspci_SRCS += devLibPCIStrings.c

epicspci_SRCS_RTEMS += osdPciShared.c
epicspci_SRCS_vxWorks += osdPciShared.c

epicspci_SRCS += devLibPCIOSD.c
epicspci_SRCS += pcish.c

epicspci_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
