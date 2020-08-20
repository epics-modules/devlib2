TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -I$(TOP)/common

USR_SYS_LIBS_WIN32 += ws2_32 advapi32 user32

# explore app doesn't build with older vxworks (eg. tornado22)
# don't build at all until someone asks
LIBRARY_IOC_DEFAULT = explorepci
LIBRARY_IOC_vxWorks = -nil-

SHRLIB_VERSION ?= $(DEVLIB2_MAJOR_VERSION).$(DEVLIB2_MINOR_VERSION)

explorepci_SRCS += devexplore.cpp
explorepci_SRCS += devexplore_irq.cpp
explorepci_SRCS += devexplore_frib.cpp
explorepci_SRCS += devexplore_util.cpp

explorepci_LIBS += epicspci
explorepci_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC_DEFAULT = explore
PROD_IOC_vxWorks = -nil-
# explore.dbd will be created and installed
DBD += explore.dbd
DBD += exploreSupport.dbd

# explore.dbd will be made up from these files:
explore_DBD += base.dbd
explore_DBD += system.dbd
explore_DBD += epicspci.dbd
explore_DBD += exploreSupport.dbd

# explore_registerRecordDeviceDriver.cpp derives from explore.dbd
explore_SRCS += explore_registerRecordDeviceDriver.cpp

explore_SRCS += exploreMain.cpp

explore_LIBS += explorepci epicspci

# Finally link to the EPICS Base libraries
explore_LIBS += $(EPICS_BASE_IOC_LIBS)

TESTPROD_HOST += testutil
testutil_SRCS += testutil.cpp

testutil_LIBS += explorepci epicspci
testutil_LIBS += $(EPICS_BASE_IOC_LIBS)
TESTS += testutil

# if VER >= 3.15.3
#  do real IOC testing
ifneq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifeq ($(findstring $(EPICS_REVISION)$(EPICS_MODIFICATION),151 152),)
TESTPROD_HOST += testexplore
DBD += testexplore.dbd
TESTS += testexplore

testexplore_DBD += base.dbd
testexplore_DBD += epicspci.dbd
testexplore_DBD += exploreSupport.dbd

testexplore_SRCS += testexplore.cpp
testexplore_SRCS += testexplore_registerRecordDeviceDriver.cpp

testexplore_LIBS += explorepci epicspci
testexplore_LIBS += $(EPICS_BASE_IOC_LIBS)

endif
endif

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
