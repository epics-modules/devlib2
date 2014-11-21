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
#include <string.h>

#include <ellLib.h>
#include <errlog.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "devLibPCIImpl.h"

#define epicsExportSharedSymbols
#include "devLibPCI.h"

#ifndef CONTAINER
# ifdef __GNUC__
#   define CONTAINER(ptr, structure, member) ({                     \
        const __typeof(((structure*)0)->member) *_ptr = (ptr);      \
        (structure*)((char*)_ptr - offsetof(structure, member));    \
    })
# else
#   define CONTAINER(ptr, structure, member) \
        ((structure*)((char*)(ptr) - offsetof(structure, member)))
# endif
#endif

int devPCIDebug = 0;

static ELLLIST pciDrivers;

static devLibPCI *pdevLibPCI;

static epicsMutexId pciDriversLock;
static epicsThreadOnceId devPCIReg_once = EPICS_THREAD_ONCE_INIT;

static epicsThreadOnceId devPCIInit_once = EPICS_THREAD_ONCE_INIT;
static int devPCIInit_result = 42;

/******************* Initialization *********************/

static
void regInit(void* junk)
{
    pciDriversLock = epicsMutexMustCreate();
}

epicsShareFunc
int
devLibPCIRegisterDriver2(devLibPCI* drv, size_t drvsize)
{
    int ret=0;
    ELLNODE *cur;

    if (!drv->name) return 1;

    if(drvsize!=sizeof(*drv)) {
        errlogPrintf("devLibPCIRegisterDriver() fails with inconsistent PCI OS struct sizes.\n"
                     "expect %lu but given %lu\n"
                     "Please do a clean rebuild of devLib2 and any code with custom PCI OS structs\n",
                     (unsigned long)sizeof(*drv),
                     (unsigned long)drvsize);
        return S_dev_internal;
    }

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);

    for(cur=ellFirst(&pciDrivers); cur; cur=ellNext(cur)) {
        devLibPCI *other=CONTAINER(cur, devLibPCI, node);
        if (strcmp(drv->name, other->name)==0) {
            errlogPrintf("Failed to register PCI bus driver: name already taken\n");
            ret=1;
            break;
        }
    }
    if (!ret)
        ellAdd(&pciDrivers, &drv->node);

    epicsMutexUnlock(pciDriversLock);

    return ret;
}

epicsShareFunc
int
devLibPCIUse(const char* use)
{
    ELLNODE *cur;
    devLibPCI *drv;

    if (!use)
        use="native";

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);

    if (pdevLibPCI) {
        epicsMutexUnlock(pciDriversLock);
        errlogPrintf("PCI bus driver already selected. Can't change selection\n");
        return 1;
    }

    for(cur=ellFirst(&pciDrivers); cur; cur=ellNext(cur)) {
        drv=CONTAINER(cur, devLibPCI, node);
        if (strcmp(drv->name, use)==0) {
            pdevLibPCI = drv;
            epicsMutexUnlock(pciDriversLock);
            return 0;
        }
    }
    epicsMutexUnlock(pciDriversLock);
    errlogPrintf("PCI bus driver '%s' not found\n",use);
    return 1;
}

epicsShareFunc
const char* devLibPCIDriverName()
{
    const char* ret=NULL;

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);
    if (pdevLibPCI)
        ret = pdevLibPCI->name;
    epicsMutexUnlock(pciDriversLock);

    return ret;
}

static
void devInit(void* junk)
{
  epicsThreadOnce(&devPCIReg_once, &regInit, NULL);
  epicsMutexMustLock(pciDriversLock);
  if(!pdevLibPCI && devLibPCIUse(NULL)) {
      epicsMutexUnlock(pciDriversLock);
      devPCIInit_result = S_dev_internal;
      return;
    }
  epicsMutexUnlock(pciDriversLock);

  if(!!pdevLibPCI->pDevInit)
    devPCIInit_result = (*pdevLibPCI->pDevInit)();
  else
    devPCIInit_result = 0;
}

#define PCIINIT \
do { \
     epicsThreadOnce(&devPCIInit_once, &devInit, NULL); \
     if (devPCIInit_result) return devPCIInit_result; \
} while(0)


/**************** API functions *****************/

epicsShareFunc
int devPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
)
{
  if(!idlist || !searchfn)
    return S_dev_badArgument;

  PCIINIT;

  return (*pdevLibPCI->pDevPCIFind)(idlist,searchfn,arg,opt);
}


struct bdfmatch
{
  unsigned int domain,b,d,f;
  const epicsPCIDevice* found;
};

static
int bdfsearch(void* ptr, const epicsPCIDevice* cur)
{
  struct bdfmatch *mt=ptr;

  if( cur->domain==mt->domain &&
      cur->bus==mt->b &&
      cur->device==mt->d &&
      cur->function==mt->f )
  {
    mt->found=cur;
    return 1;
  }

  return 0;
}

/*
 * The most common PCI search using only id fields and BDF.
 */
epicsShareFunc
int devPCIFindDBDF(
     const epicsPCIID *idlist,
     unsigned int      domain,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
const epicsPCIDevice **found,
     unsigned int      opt
)
{
  int err;
  struct bdfmatch find;

  if(!found)
    return 2;

  find.domain=domain;
  find.b=b;
  find.d=d;
  find.f=f;
  find.found=NULL;

  /* PCIINIT is called by devPCIFindCB()  */

  err=devPCIFindCB(idlist,&bdfsearch,&find, opt);
  if(err!=0){
    /* Search failed? */
    return err;
  }

  if(!find.found){
    /* Not found */
    return S_dev_noDevice;
  }

  *found=find.found;
  return 0;
}

/* for backward compatilility: b=domain*0x100+bus */
epicsShareFunc
int devPCIFindBDF(
     const epicsPCIID *idlist,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
const epicsPCIDevice **found,
     unsigned int      opt
)
{
    return devPCIFindDBDF(idlist, b>>8, b&0xff, d, f, found, opt);
}

int
devPCIToLocalAddr(
  const epicsPCIDevice *curdev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
)
{
  PCIINIT;

  if(bar>=PCIBARCOUNT)
    return S_dev_badArgument;

  return (*pdevLibPCI->pDevPCIToLocalAddr)(curdev,bar,ppLocalAddr,opt);
}



epicsShareFunc
int
devPCIBarLen(
  const epicsPCIDevice *curdev,
          unsigned int  bar,
          epicsUInt32 *len
)
{
  PCIINIT;

  if(bar>=PCIBARCOUNT)
    return S_dev_badArgument;

  return (*pdevLibPCI->pDevPCIBarLen)(curdev,bar,len);
}

epicsShareFunc
int devPCIConnectInterrupt(
  const epicsPCIDevice *curdev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
  PCIINIT;

  return (*pdevLibPCI->pDevPCIConnectInterrupt)
                (curdev,pFunction,parameter,opt);
}

epicsShareFunc
int devPCIDisconnectInterrupt(
  const epicsPCIDevice *curdev,
  void (*pFunction)(void *),
  void  *parameter
)
{
  PCIINIT;

  return (*pdevLibPCI->pDevPCIDisconnectInterrupt)
                (curdev,pFunction,parameter);
}

typedef struct {
    int lvl;
    int matched;
} searchinfo;

static
int
searchandprint(void* praw,const epicsPCIDevice* dev)
{
    searchinfo *pinfo=praw;
    pinfo->matched++;
    devPCIShowDevice(pinfo->lvl,dev);
    errlogFlush(); /* avoid truncation for long device lists */
    return 0;
}

void
devPCIShow(int lvl, int vendor, int device, int exact)
{
    epicsPCIID ids[] = {
        DEVPCI_DEVICE_VENDOR(device,vendor),
        DEVPCI_END
    };
    searchinfo info;
    info.lvl=lvl;
    info.matched=0;

    if (vendor==0 && !exact) ids[0].vendor=DEVPCI_ANY_VENDOR;
    if (device==0 && !exact) ids[0].device=DEVPCI_ANY_DEVICE;

    devPCIFindCB(ids,&searchandprint, &info, 0);
    errlogPrintf("Matched %d devices\n", info.matched);
}

void
devPCIShowDevice(int lvl, const epicsPCIDevice *dev)
{
    int i;

    errlogPrintf("PCI %04x:%02x:%02x.%x IRQ %u\n"
           "  vendor:device %04x:%04x rev %02x\n",
           dev->domain, dev->bus, dev->device, dev->function, dev->irq,
           dev->id.vendor, dev->id.device, dev->id.revision);
    if(lvl<1)
        return;
    errlogPrintf("  subved:subdev %04x:%04x\n"
           "  class %06x %s\n",
           dev->id.sub_vendor, dev->id.sub_device,
           dev->id.pci_class,
           devPCIDeviceClassToString(dev->id.pci_class));
    if (dev->driver) errlogPrintf("  driver %s\n",
           dev->driver);
    if(lvl<2)
        return;
    for(i=0; i<PCIBARCOUNT; i++)
    {
        epicsUInt32 len;

        if ((*pdevLibPCI->pDevPCIBarLen)(dev, i, &len) == 0 && len > 0)
        {
            char* u = "";
            if (len >= 1024) { len >>= 10; u = "k"; }
            if (len >= 1024) { len >>= 10; u = "M"; }
            if (len >= 1024) { len >>= 10; u = "G"; }

            errlogPrintf("  BAR %u %s-bit %s%s %3u %sB\n",i,
                   dev->bar[i].addr64?"64":"32",
                   dev->bar[i].ioport?"IO Port":"MMIO   ",
                   dev->bar[i].below1M?" Below 1M":"",
                   len, u);
        }
        /* 64 bit bars use 2 entries */
        if (dev->bar[i].addr64) i++;
    }
}

static int
checkCfgAccess(const epicsPCIDevice *dev, unsigned offset, void *arg, devPCIAccessMode mode)
{
    int rval;

    if ( (offset & (CFG_ACC_WIDTH(mode) - 1)) )
        return S_dev_badArgument; /* misaligned      */
    if ( ! pdevLibPCI->pDevPCIConfigAccess )
        return S_dev_badFunction; /* not implemented */

    rval = (*pdevLibPCI->pDevPCIConfigAccess)(dev, offset, arg, mode);

    if ( rval )
        return rval;

    return 0;
}

int
devPCIConfigRead8(const epicsPCIDevice *dev, unsigned offset, epicsUInt8 *pResult)
{
    return checkCfgAccess(dev, offset, pResult, RD_08);
}

int
devPCIConfigRead16(const epicsPCIDevice *dev, unsigned offset, epicsUInt16 *pResult)
{
    return checkCfgAccess(dev, offset, pResult, RD_16);
}

int 
devPCIConfigRead32(const epicsPCIDevice *dev, unsigned offset, epicsUInt32 *pResult)
{
    return checkCfgAccess(dev, offset, pResult, RD_32);
}

int
devPCIConfigWrite8(const epicsPCIDevice *dev, unsigned offset, epicsUInt8 value)
{
    return checkCfgAccess(dev, offset, &value, WR_08);
}

int
devPCIConfigWrite16(const epicsPCIDevice *dev, unsigned offset, epicsUInt16 value)
{
    return checkCfgAccess(dev, offset, &value, WR_16);
}

int 
devPCIConfigWrite32(const epicsPCIDevice *dev, unsigned offset, epicsUInt32 value)
{
    return checkCfgAccess(dev, offset, &value, WR_32);
}


int
devPCIEnableInterrupt(const epicsPCIDevice *dev)
{
    if ( ! pdevLibPCI->pDevPCIConfigAccess )
        return S_dev_badFunction; /* not implemented */

    return pdevLibPCI->pDevPCISwitchInterrupt(dev, 0);
}

int
devPCIDisableInterrupt(const epicsPCIDevice *dev)
{
    if ( ! pdevLibPCI->pDevPCIConfigAccess )
        return S_dev_badFunction; /* not implemented */

    return pdevLibPCI->pDevPCISwitchInterrupt(dev, 1);
}


static const iocshArg devPCIShowArg0 = { "verbosity level",iocshArgInt};
static const iocshArg devPCIShowArg1 = { "PCI Vendor ID (0=any)",iocshArgInt};
static const iocshArg devPCIShowArg2 = { "PCI Device ID (0=any)",iocshArgInt};
static const iocshArg devPCIShowArg3 = { "exact (1=treat 0 as 0)",iocshArgInt};
static const iocshArg * const devPCIShowArgs[4] =
{&devPCIShowArg0,&devPCIShowArg1,&devPCIShowArg2,&devPCIShowArg3};
static const iocshFuncDef devPCIShowFuncDef =
    {"devPCIShow",4,devPCIShowArgs};
static void devPCIShowCallFunc(const iocshArgBuf *args)
{
    devPCIShow(args[0].ival,args[1].ival,args[2].ival,args[3].ival);
}

static const iocshArg devLibPCIUseArg0 = { "verbosity level",iocshArgString};
static const iocshArg * const devLibPCIUseArgs[1] =
{&devLibPCIUseArg0};
static const iocshFuncDef devLibPCIUseFuncDef =
    {"devLibPCIUse",1,devLibPCIUseArgs};
static void devLibPCIUseCallFunc(const iocshArgBuf *args)
{
    devLibPCIUse(args[0].sval);
}

#include <epicsExport.h>

static
void devLibPCIIOCSH()
{
  iocshRegister(&devPCIShowFuncDef,devPCIShowCallFunc);
  iocshRegister(&devLibPCIUseFuncDef,devLibPCIUseCallFunc);
}

epicsExportRegistrar(devLibPCIIOCSH);

epicsExportAddress(int,devPCIDebug);
