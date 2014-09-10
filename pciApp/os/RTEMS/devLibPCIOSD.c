
#include <stdlib.h>
#include <epicsAssert.h>
#include <epicsMutex.h>
#include <epicsInterrupt.h>
#include <errlog.h>

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <rtems/irq.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"

/* Provide a weak symbol for BSP_pci_configuration. If the BSP does not
 * support PCI then BSP_pci_configuration resolves to the weak symbol.
 * If the BSP does support PCI then the 'true'/strong symbol is used
 * IF and ONLY IF the BSP enforces linking it (a strong symbol is not
 * pulled out of a library if a weak symbol has already be found. However,
 * if the strong symbol is linked anyways (due to some other dependency
 * e.g., 'pci_initialize()' from the same compilation unit) then it does
 * override the weak one.
 */

#ifdef __rtems__
#include <rtems.h>
#if !defined(__RTEMS_MAJOR__) || !defined(__RTEMS_MINOR__)
#error "Unkown RTEMS version -- missing header?"
#endif

#if (__RTEMS_MAJOR__ < 4) || (__RTEMS_MAJOR__ == 4 && __RTEMS_MINOR__ <= 9)
#define rtems_pci_config_t pci_config
#endif

extern rtems_pci_config_t BSP_pci_configuration __attribute__((weak));

#endif

static
int rtemsDevPCIConnectInterrupt(
  const epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
    struct osdPCIDevice *id=pcidev2osd(dev);

    rtems_irq_connect_data isr;

    isr.name = id->dev.irq;
    isr.hdl = pFunction;
    isr.handle = parameter;

    isr.on = NULL;
    isr.off= NULL;
    isr.isOn=NULL;

#ifdef BSP_SHARED_HANDLER_SUPPORT
    isr.next_handler=NULL;

    if (!BSP_install_rtems_shared_irq_handler(&isr))
        return S_dev_vecInstlFail;
#else
    if (!BSP_install_rtems_irq_handler(&isr))
        return S_dev_vecInstlFail;
#endif

    return 0;
}

static
int rtemsDevPCIDisconnectInterrupt(
  const epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter
)
{
    struct osdPCIDevice *id=pcidev2osd(dev);

    rtems_irq_connect_data isr;

    isr.name = id->dev.irq;
    isr.hdl = pFunction;
    isr.handle = parameter;

    isr.on = NULL;
    isr.off= NULL;
    isr.isOn=NULL;

#ifdef BSP_SHARED_HANDLER_SUPPORT
    isr.next_handler=NULL;
#endif

    if(!BSP_remove_rtems_irq_handler(&isr))
        return S_dev_intDisconnect;

    return 0;
}

static int
rtemsDevPCIConfigAccess(const epicsPCIDevice *dev, unsigned offset, void *pArg, devPCIAccessMode mode)
{
    int           rval    = S_dev_internal;
    long          flags;

    if ( 0 == & BSP_pci_configuration ) {
        /* BSP has no PCI support */
        return S_dev_badRequest;
    }

    /* RTEMS (as of 4.10) does NOT have any kind of protection against races
     * for the single, shared and global resource (indirect register pair,
     * pcibios, ...) which does config-space transactions!
     *
     * By locking interrupts here we make it at least safe to use the access
     * functions from within the framework of devlib2.
     *
     * We lock interrupts so that we may use access functions from ISRs.
     * The downside is a possible latency-penalty (e.g., the i386 implementation
     * uses the pci-bios of the machine and nobody knows how fast or slow
     * that is...)
     */

    flags = epicsInterruptLock();

    rval = sharedDevPCIConfigAccess(dev, offset, pArg, mode);

    epicsInterruptUnlock(flags);

    return rval;
}

devLibPCI prtemsPCI = {
  "native",
  sharedDevPCIInit, NULL,
  sharedDevPCIFindCB,
  sharedDevPCIToLocalAddr,
  sharedDevPCIBarLen,
  rtemsDevPCIConnectInterrupt,
  rtemsDevPCIDisconnectInterrupt,
  rtemsDevPCIConfigAccess
};

#include <epicsExport.h>

void devLibPCIRegisterBaseDefault(void)
{
    devLibPCIRegisterDriver(&prtemsPCI);
}
epicsExportRegistrar(devLibPCIRegisterBaseDefault);
