
#include <stdlib.h>
#include <epicsAssert.h>

#include <vxWorks.h>
#include <types.h>
#include <sysLib.h>
#include <intLib.h>
#include <iv.h>
#include <drv/pci/pciIntLib.h>
#include <vxLib.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"

#if defined(VXPCIINTOFFSET)
/* do nothing */

#elif defined(INT_NUM_IRQ0)
#define VXPCIINTOFFSET INT_NUM_IRQ0

#elif defined(INT_VEC_IRQ0)
#define VXPCIINTOFFSET INT_VEC_IRQ0

#else
#define VXPCIINTOFFSET 0

#endif

static
int vxworksDevPCIConnectInterrupt(
  const epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
  int status;
  struct osdPCIDevice *osd=pcidev2osd(dev);

  status=pciIntConnect((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction, (int)parameter);

  if(status)
    return S_dev_vecInstlFail;

  return 0;
}

static
int vxworksDevPCIDisconnectInterrupt(
  const epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter
)
{
  int status;
  struct osdPCIDevice *osd=pcidev2osd(dev);

#ifdef VXWORKS_PCI_OLD

  status=pciIntDisconnect((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction);

#else

  status=pciIntDisconnect2((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction, (int)parameter);

#endif

  if(status)
    return S_dev_intDisconnect;

  return 0;
}

static
int vxworksPCIToLocalAddr(const epicsPCIDevice* dev,
                          unsigned int bar, volatile void **loc,
                          unsigned int o)
{
  int ret, space=0;
  volatile void *pci;

  ret=sharedDevPCIToLocalAddr(dev, bar, &pci, o);

  if(ret)
    return ret;

#if defined(PCI_SPACE_MEMIO_PRI)
  if(!dev->bar[bar].ioport) {
    space = PCI_SPACE_MEMIO_PRI;
  }
#endif

#if defined(PCI_SPACE_IO_PRI)
  if(dev->bar[bar].ioport) {
    space = PCI_SPACE_IO_PRI;
  }
#endif

  if(space) {
    if(sysBusToLocalAdrs(space, (char*)pci, (char**)loc))
      return -1;
  } else {
    *loc=pci;
  }

  return 0;
}

devLibPCI pvxworksPCI = {
  "native",
  sharedDevPCIInit, NULL,
  sharedDevPCIFindCB,
  vxworksPCIToLocalAddr,
  sharedDevPCIBarLen,
  vxworksDevPCIConnectInterrupt,
  vxworksDevPCIDisconnectInterrupt
};
#include <epicsExport.h>

void devLibPCIRegisterBaseDefault(void)
{
    devLibPCIRegisterDriver(&pvxworksPCI);
}
epicsExportRegistrar(devLibPCIRegisterBaseDefault);
