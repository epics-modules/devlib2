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

TARGETS += $(COMMON_DIR)/exploretest.dbd
DBDDEPENDS_FILES += exploretest.dbd$(DEP)
exploretest_DBD += base.dbd
exploretest_DBD += epicspci.dbd
exploretest_DBD += exploreSupport.dbd
TESTFILES += $(COMMON_DIR)/exploretest.dbd

TESTPROD_HOST += exploretest
exploretest_SRCS += exploretest.c
exploretest_SRCS += exploretest_registerRecordDeviceDriver.cpp
exploretest_LIBS += epicspci
exploretest_LIBS += explorepci
exploretest_LIBS += $(EPICS_BASE_IOC_LIBS)
TESTFILES += ../exploretest.db

PROD_SRCS_RTEMS += rtemsTestData.c

rtemsTestData.c : $(TESTFILES) $(TOOLS)/epicsMakeMemFs.pl
	$(PERL) $(TOOLS)/epicsMakeMemFs.pl $@ epicsRtemsFSImage $(TESTFILES)

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

