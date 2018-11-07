TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_LIBS += Com

PROD_SYS_LIBS_WIN32 += ws2_32 advapi32 user32

TESTPROD_HOST += pcitest
TESTS += pcitest

pcitest_SRCS += pcitest.c
pcitest_LIBS += epicspci

TESTPROD_HOST += epicsMMIOTest
TESTS += epicsMMIOTest

epicsMMIOTest_SRCS += epicsMMIOTest.c

TESTPROD_HOST += lspcix
lspcix_SRCS += lspcix.c
lspcix_LIBS += epicspci

TESTPROD_HOST += pciio
pciio_SRCS += pciio.c
pciio_LIBS += epicspci

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

# RTEMS/QEMU testing infrastructure added in 7.0.2
ifdef BASE_7_0
ifneq ($(EPICS_MODIFICATION),1)
PROD_SRCS_RTEMS += rtemsTestData.c

ifneq ($(filter $(T_A),$(CROSS_COMPILER_RUNTEST_ARCHS)),)
TESTPROD_RTEMS = $(TESTPROD_HOST)
TESTSCRIPTS_RTEMS += $(TESTS:%=%.t)
endif

endif
endif # ifdef BASE_7_0

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

