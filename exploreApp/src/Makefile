TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = explore
# explore.dbd will be created and installed
DBD += explore.dbd

# explore.dbd will be made up from these files:
explore_DBD += base.dbd
explore_DBD += system.dbd
explore_DBD += epicspci.dbd
explore_DBD += exploreSupport.dbd

# explore_registerRecordDeviceDriver.cpp derives from explore.dbd
explore_SRCS += explore_registerRecordDeviceDriver.cpp

explore_SRCS += exploreMain.cpp
explore_SRCS += devexplore.c
explore_SRCS += devexplore2.cpp

explore_LIBS += epicspci

# Finally link to the EPICS Base libraries
explore_LIBS += $(EPICS_BASE_IOC_LIBS)


# if VER >= 3.15.3
#  do real IOC testing
ifneq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifeq ($(findstring $(EPICS_REVISION)$(EPICS_MODIFICATION),151 152),)
TESTPROD_HOST = testexplore
DBD += testexplore.dbd
TESTS += testexplore

testexplore_DBD += base.dbd
testexplore_DBD += epicspci.dbd
testexplore_DBD += exploreSupport.dbd

testexplore_SRCS += testexplore.cpp
testexplore_SRCS += testexplore_registerRecordDeviceDriver.cpp
testexplore_SRCS += devexplore.c
testexplore_SRCS += devexplore2.cpp
testexplore_LIBS += epicspci
testexplore_LIBS += $(EPICS_BASE_IOC_LIBS)

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

else
$(error x$(findstring $(EPICS_REVISION)$(EPICS_MODIFICATION),151 152))
endif
else
$(error y$(EPICS_VERSION)$(EPICS_REVISION))
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
