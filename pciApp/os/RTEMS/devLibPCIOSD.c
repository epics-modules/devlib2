
#include <stdlib.h>
#include <epicsAssert.h>
#include <epicsMutex.h>
#include <errlog.h>

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <bsp/irq.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"

static epicsMutexId rtemsGuard;

/* Provide a weak symbol for BSP_pci_configuration. If the BSP does not
 * support PCI then BSP_pci_configuration resolves to the weak symbol.
 * If the BSP does support PCI then the 'true'/strong symbol is used
 * IF and ONLY IF the BSP enforces linking it (a strong symbol is not
 * pulled out of a library if a weak symbol has already be found. However,
 * if the strong symbol is linked anyways (due to some other dependency
 * e.g., 'pci_initialize()' from the same compilation unit) then it does
 * override the weak one.
 */

extern rtems_pci_config_t BSP_pci_configuration __attribute__((weak));

static int
rtemsDevPCIInit(void)
{
int rval;

	rval = sharedDevPCIInit();

	rtemsGuard = epicsMutexMustCreate();

	return rval;
}

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
rtemsDevPCIConfigAccess(const epicsPCIDevice *dev, unsigned offset, void *pArg, DevLibPCIAccMode mode)
{
int           rval    = S_dev_internal;
int           st;

	if ( 0 == & BSP_pci_configuration ) {
		/* BSP has no PCI support */
		return S_dev_badRequest;
	}

    if(epicsMutexLock(rtemsGuard)!=epicsMutexLockOK)
        return S_dev_internal;

	if ( CFG_ACC_WRITE(mode) ) {
		switch ( CFG_ACC_WIDTH(mode) ) {
			default:
			case 1:
				st = pci_write_config_byte( dev->bus, dev->device, dev->function, (unsigned char)offset, *(uint8_t*)pArg );
			break;

			case 2:
				st = pci_write_config_word( dev->bus, dev->device, dev->function, (unsigned char)offset, *(uint16_t*)pArg );
			break;
			case 4:
				st = pci_write_config_dword( dev->bus, dev->device, dev->function, (unsigned char)offset, *(uint32_t*)pArg );
			break;
		}
	} else {
		switch ( CFG_ACC_WIDTH(mode) ) {
			default:
			case 1:
				st = pci_read_config_byte( dev->bus, dev->device, dev->function, (unsigned char)offset, pArg );
			break;

			case 2:
				st = pci_read_config_word( dev->bus, dev->device, dev->function, (unsigned char)offset, pArg );
			break;
			case 4:
				st = pci_read_config_dword( dev->bus, dev->device, dev->function, (unsigned char)offset, pArg );
			break;
		}
	}

	if ( st ) {
		errlogPrintf("devLibPCIOSD: Unable to %s %u bytes %s configuration space: PCIBIOS error code 0x%02x\n",
			             CFG_ACC_WRITE(mode) ? "write" : "read",
			             CFG_ACC_WIDTH(mode),
			             CFG_ACC_WRITE(mode) ? "to" : "from",
			             st);
                       
		rval = S_dev_internal;
	} else {
		rval = 0;
	}

    epicsMutexUnlock(rtemsGuard);

	return rval;
}

devLibPCI prtemsPCI = {
  "native",
  rtemsDevPCIInit, NULL,
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
