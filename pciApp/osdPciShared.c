/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>

#include <errlog.h>
#include <epicsMutex.h>
#include <epicsInterrupt.h>

#include "devLibPCIImpl.h"

#include "devLibPCI.h"

#define epicsExportSharedSymbols
#include "osdPciShared.h"

/* List of osdPCIDevice */
static ELLLIST devices;

int
sharedDevPCIInit(void)
{
    int b, d, f, bar;
    osdPCIDevice *next;
    uint8_t val8, header;
    PCIUINT32 val32;

    /*
     * Ensure all entries for the requested device/vendor pairs
     * are in the 'devices' list.
     * This function runs in single threaded context.
     * Later the device list never changes.
     * Thus there is no need for a semaphore.
     */

    /* Read config space */
    for (b=0; b<256; b++)
      for (d=0; d<32; d++)
        for (f=0; f<8; f++) {
          /* no special bus cycle */
          if (d == 31 && f == 7)
              continue;

          /* check for existing device */
          pci_read_config_dword(b,d,f,PCI_VENDOR_ID, &val32);
          if (val32 == 0xffffffff)
              break;

          if (devPCIDebug >= 1)
            errlogPrintf("sharedDevPCIInit found %d.%d.%d: %08x\n",b,d,f, (unsigned)val32);

          next=calloc(1,sizeof(osdPCIDevice));
          if (!next)
            return S_dev_noMemory;

          next->dev.bus = b;
          next->dev.device = d;
          next->dev.function = f;
          next->dev.id.vendor = val32&0xffff;
          next->dev.id.device = val32>>16;

          pci_read_config_dword(b,d,f,PCI_SUBSYSTEM_VENDOR_ID, &val32);
          next->dev.id.sub_vendor = val32&0xffff;
          next->dev.id.sub_device = val32>>16;

          pci_read_config_dword(b,d,f,PCI_CLASS_REVISION, &val32);
          next->dev.id.revision = val32&0xff;
          next->dev.id.pci_class = val32>>8;

          if(devPCIDebug>=1)
            errlogPrintf(" as pri %04x:%04x sub %04x:%04x cls %06x\n",
                         next->dev.id.vendor, next->dev.id.device,
                         next->dev.id.sub_vendor, next->dev.id.sub_device,
                         next->dev.id.pci_class);

	  pci_read_config_byte(b,d,f,PCI_HEADER_TYPE, &header);
          for (bar=0;bar<PCIBARCOUNT;bar++) {
            if (bar>=2 && (header & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE)
            {
              if(devPCIDebug>=1)
                  errlogPrintf(" bridge device\n");
              break;
            }
            pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &val32);
            next->dev.bar[bar].ioport = (val32 & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO;
            if (next->dev.bar[bar].ioport) {
              /* This BAR is I/O ports */
              next->base[bar] = val32 & PCI_BASE_ADDRESS_IO_MASK;
            } else {
              /* This BAR is memory mapped */
              next->dev.bar[bar].below1M = !!(val32&PCI_BASE_ADDRESS_MEM_TYPE_1M);
              next->dev.bar[bar].addr64 = !!(val32&PCI_BASE_ADDRESS_MEM_TYPE_64);
              next->base[bar] = val32 & PCI_BASE_ADDRESS_MEM_MASK;
              /* TODO: Take care of 64 bit BARs! */
              if (next->dev.bar[bar].addr64)
              {
                  bar++;
                  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &val32);
                  next->base[bar] = val32;
              }
            }
          }

          pci_read_config_dword(b,d,f,PCI_ROM_ADDRESS, &val32);
          next->erom = val32 & PCI_ROM_ADDRESS_MASK;

          pci_read_config_byte(b,d,f,PCI_INTERRUPT_LINE, &val8);
          next->dev.irq = val8;

          ellInsert(&devices,ellLast(&devices),&next->node);

	  if (f == 0 && !(header & PCI_HEADER_MULTI_FUNC))
          {
            if(devPCIDebug>=1)
              errlogPrintf(" single function device\n");

            break;
          }
    }
    return 0;
}

/*
 * Machinery for searching for PCI devices.
 *
 * This is a general function to support all possible
 * search filtering conditions.
 */
int
sharedDevPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
)
{
  int err=0;
  ELLNODE *cur;
  osdPCIDevice *curdev=NULL;
  const epicsPCIID *search;

  if (devPCIDebug>=1)
    errlogPrintf("sharedDevPCIFindCB\n");

  if(!searchfn || !idlist)
    return S_dev_badArgument;

  cur=ellFirst(&devices);
  for(; cur; cur=ellNext(cur)){
    curdev=CONTAINER(cur,osdPCIDevice,node);

    for(search=idlist; search && !!search->device; search++){

      if (search->device!=DEVPCI_ANY_DEVICE &&
          search->device!=curdev->dev.id.device)
        continue;
      else
      if (search->vendor!=DEVPCI_ANY_VENDOR &&
          search->vendor!=curdev->dev.id.vendor)
        continue;
      else
      if (search->sub_device!=DEVPCI_ANY_SUBDEVICE &&
          search->sub_device!=curdev->dev.id.sub_device)
        continue;
      else
      if (search->sub_vendor!=DEVPCI_ANY_SUBVENDOR &&
          search->sub_vendor!=curdev->dev.id.sub_vendor)
        continue;
      else
      if (search->pci_class!=DEVPCI_ANY_CLASS &&
          search->pci_class!=curdev->dev.id.pci_class)
        continue;
      else
      if (search->revision!=DEVPCI_ANY_REVISION &&
          search->revision!=curdev->dev.id.revision)
        continue;

      /* Match found */

      err=searchfn(arg,&curdev->dev);
      switch(err){
      case 0: /* Continue search */
        break;
      case 1: /* Abort search OK */
        err=0;
      default:/* Abort search Err */
        goto done;
      }
    }
  }

  err=0;
done:
  return err;
}

int
sharedDevPCIToLocalAddr(
  const epicsPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
)
{
  struct osdPCIDevice *osd=pcidev2osd(dev);

  /* No locking since the base address is not changed
   * after the osdPCIDevice is created
   */

  if(!osd->base[bar])
    return S_dev_addrMapFail;
  
  if (dev->bar[bar].addr64)
  {
#if __SIZEOF_POINTER__ > 4
    *ppLocalAddr=(volatile void*)(osd->base[bar] | (long long)osd->base[bar+1] << 32);
    return 0;
#else
    if (osd->base[bar+1])
    {
      errlogPrintf("sharedDevPCIToLocalAddr: Unable map a 64 bit BAR on a 32 bit system");
      return S_dev_addrMapFail;
    }
#endif
  }
  *ppLocalAddr=(volatile void*)osd->base[bar];
  
  return 0;
}

int
sharedDevPCIBarLen(
  const epicsPCIDevice* dev,
  unsigned int bar,
  epicsUInt32 *len
)
{
  struct osdPCIDevice *osd=pcidev2osd(dev);
  int b=dev->bus, d=dev->device, f=dev->function;
  PCIUINT32 start, max, mask;
  long iflag;

  if(!osd->base[bar])
    return S_dev_badSignalNumber;

  /* Disable interrupts since we are changing a device's PCI BAR
   * register.  This is not safe to do on an active device.
   * Disabling interrupts avoids some, but not all, of these problems
   */
  iflag=epicsInterruptLock();

  if(osd->len[bar]) {
    *len=osd->len[bar];
    epicsInterruptUnlock(iflag);
    return 0;
  }

  /* Note: the following assumes the bar is 32-bit */

  if(dev->bar[bar].ioport)
    mask=PCI_BASE_ADDRESS_IO_MASK;
  else
    mask=PCI_BASE_ADDRESS_MEM_MASK;

  /*
   * The idea here is to find the least significant bit which
   * is set by writing 1 to all the address bits.
   *
   * For example the mask for 32-bit IO Memory is 0xfffffff0
   * If a base address is currently set to 0x00043000
   * and when the mask is written the address becomes
   * 0xffffff80 then the length is 0x80 (128) bytes
   */
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &start);

  /* If the BIOS didn't set this BAR then don't mess with it */
  if((start&mask)==0) {
    epicsInterruptUnlock(iflag);
    return S_dev_badRequest;
  }

  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), mask);
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &max);
  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), start);

  /* mask out bits which aren't address bits */
  max&=mask;

  /* Find lsb */
  osd->len[bar] = max & ~(max-1);

  *len=osd->len[bar];
  epicsInterruptUnlock(iflag);
  return 0;
}

int
sharedDevPCIConfigAccess(const epicsPCIDevice *dev, unsigned offset, void *pArg, devPCIAccessMode mode)
{
    int st = 1;

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

    if (st) {
        errlogPrintf("devLibPCIOSD: Unable to %s %u bytes %s configuration space: PCIBIOS error code 0x%02x\n",
                     CFG_ACC_WRITE(mode) ? "write" : "read",
                     CFG_ACC_WIDTH(mode),
                     CFG_ACC_WRITE(mode) ? "to" : "from",
                     st);

        return S_dev_internal;
    } else {
        return 0;
    }
}
