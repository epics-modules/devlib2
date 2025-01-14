/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <string.h>

#include <errlog.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include "devLibPCI.h"

#define testEQString(A, B) testOk(strcmp(A,B)==0, "'%s' == '%s'", A, B)

extern void devLibPCIRegisterBaseDefault(void);

static void classStrings(void) {
    testDiag("Test PCI class lookup table");
    testEQString(devPCIDeviceClassToString(0x123400), "unknown processing accelerator");
    testEQString(devPCIDeviceClassToString(0xfffe00), "unknown device class");
    testEQString(devPCIDeviceClassToString(0xffff00), "unknown device class");
    testEQString(devPCIDeviceClassToString(0x0b8000), "generic processor");
    testEQString(devPCIDeviceClassToString(0x0d0000), "IRDA controller");
    testEQString(devPCIDeviceClassToString(0x060000), "PCI host bridge");
}

static const epicsPCIID hostbridge[] = {
    DEVPCI_DEVICE_VENDOR_CLASS(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR, 0x060000),
    DEVPCI_END
};

static int foundbridges;

static int showbridge(void* ptr,const epicsPCIDevice* dev)
{
    testDiag("Found bridge at %d:%d:%d.%d", dev->domain, dev->bus, dev->device, dev->function);
    foundbridges++;
    return 0;
}

static void findRootBridge(void) {
    const epicsPCIDevice *dev = NULL,
                         *dev2 = NULL;
    epicsUInt16 val;

    testDiag("Find host bridges");
    testOk1(devPCIFindCB(hostbridge, &showbridge, NULL, 0)==0);
    testOk1(foundbridges>0);

    testDiag("Get root bridge");
    testOk1(devPCIFindDBDF(hostbridge, 0, 0, 0, 0, &dev, 0)==0);
    if(!dev) {
        testFail("Didn't find root bridge");
        testSkip(15, "No bridge");
        return;
    } else {
        testPass("Found root bridge %04x:%04x", dev->id.vendor, dev->id.device);
    }

    testOk1(devPCIConfigRead16(dev, 0, &val)==0);
    testOk1(dev->id.vendor==val);
    testOk1(devPCIConfigRead16(dev, 2, &val)==0);
    testOk1(dev->id.device==val);

    dev2 = NULL;
    testOk1(devPCIFindSpec(hostbridge, "", &dev2, 0)==0);
    testOk(dev==dev2, "%x:%x.%x == %x:%x.%x",
           dev->bus, dev->device, dev->function,
           dev2->bus, dev2->device, dev2->function);

    dev2 = NULL;
    testOk1(devPCIFindSpec(hostbridge, "instance=1", &dev2, 0)==0);
    testOk(dev==dev2, "%x:%x.%x == %x:%x.%x",
           dev->bus, dev->device, dev->function,
           dev2->bus, dev2->device, dev2->function);

    dev2 = NULL;
    testOk1(devPCIFindSpec(hostbridge, "0:0.0", &dev2, 0)==0);
    testOk(dev==dev2, "%x:%x.%x == %x:%x.%x",
           dev->bus, dev->device, dev->function,
           dev2->bus, dev2->device, dev2->function);

    testDiag("Unknown qualifiers are ignored");
    dev2 = NULL;
    testOk1(devPCIFindSpec(hostbridge, "foo=bar", &dev2, 0)==0);
    testOk(dev==dev2, "%x:%x.%x == %x:%x.%x",
           dev->bus, dev->device, dev->function,
           dev2->bus, dev2->device, dev2->function);

    testOk1(devPCIFindSpec(hostbridge, "instance=2", &dev2, 0)!=0);

    testOk1(devPCIFindSpec(hostbridge, "slot=42", &dev2, 0)!=0);

    //testOk1(devPCIFindSpec(hostbridge, "foo", &dev2, 0)!=0);
}

MAIN(pcitest) {
    testPlan(24);
    devLibPCIRegisterBaseDefault();
    devLibPCIUse(NULL);
    testDiag("Using driver: %s", devLibPCIDriverName());
    /*testDiag("Enumerate PCI devices\n");
    devPCIShow(1,0,0,0);
    errlogFlush();*/
    classStrings();
#ifdef _WIN32
    testTodoBegin("PCI Not implemented");
#endif
    findRootBridge();
    testTodoEnd();
    return testDone();
}
