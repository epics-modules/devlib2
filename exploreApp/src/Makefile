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
explore_DBD += exploreSupport.dbd

# explore_registerRecordDeviceDriver.cpp derives from explore.dbd
explore_SRCS += explore_registerRecordDeviceDriver.cpp

explore_SRCS += exploreMain.cpp
explore_SRCS += devexplore.c

explore_LIBS += epicspci

# Finally link to the EPICS Base libraries
explore_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

