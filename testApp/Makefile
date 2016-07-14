TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_LIBS += Com

TESTPROD_IOC += pcitest
TESTS += pcitest

pcitest_SRCS += pcitest.c
pcitest_LIBS += epicspci

TESTPROD_IOC += epicsMMIOTest
TESTS += epicsMMIOTest

epicsMMIOTest_SRCS += epicsMMIOTest.c

TESTPROD_HOST += lspcix
lspcix_SRCS += lspcix.c
lspcix_LIBS += epicspci

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

