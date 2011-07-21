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

#include <epicsVersion.h>
#include <epicsAssert.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsMMIO.h>
#include <devLibPCI.h>

static
volatile void *diagbase=0;

static
volatile epicsUInt32 diaglen=0;

struct bdf {
    int b,d,f;
    const epicsPCIDevice *dev;
};

static
int matchbdf(void* raw,const epicsPCIDevice* dev)
{
    struct bdf *loc=raw;

    if(dev->bus!=loc->b || dev->device!=loc->d || dev->function!=loc->f)
        return 0;
    loc->dev=dev;
    return 1;
}

void pcidiagset(int b, int d, int f, int bar, int vendor, int device, int exact)
{
    epicsUInt32 len=0;
    struct bdf loc;
    epicsPCIID ids[] = {
        DEVPCI_DEVICE_VENDOR(device,vendor),
        DEVPCI_END
    };

    printf("Looking for %u:%u.%u\n", b, d, f);

    if(vendor==0 && !exact)
        ids[0].vendor=DEVPCI_ANY_VENDOR;
    if(device==0 && !exact)
        ids[0].device=DEVPCI_ANY_DEVICE;

    loc.b=b;
    loc.d=d;
    loc.f=f;
    loc.dev=0;

    if(devPCIFindCB(ids, &matchbdf, (void*)&loc, 0)) {
        printf("Error searching\n");
        return;
    }

    if(!loc.dev) {
        printf("No such device\n");
        return;
    }

    printf("Mapping %u:%u.%u\n", loc.dev->bus, loc.dev->device, loc.dev->function);

#if defined(linux)
    if(devPCIBarLen(loc.dev, bar, &len)) {
        printf("Failed to get BAR length\n");
        len=0;
    }
#endif

    if(devPCIToLocalAddr(loc.dev, bar, &diagbase, 0)) {
        printf("Failed to map BAR\n");
        return;
    }
    diaglen=len;

#if defined(linux)
    printf("BAR %u from 0x%08lx for %u bytes\n",bar, (unsigned long)diagbase, diaglen);
#else
    printf("BAR %u from 0x%08lx\n",bar, (unsigned long)diagbase);
#endif

}

void pciread(int dmod, int offset, int count)
{
  epicsUInt32 tval;
  volatile char* dptr;
  short dbytes;
  int i;

  if(!diagbase) {
      printf("Run pcidiagset first\n");
      return;
  }

  switch(dmod){
  case 8:
  case 16:
  case 32:
      break;
  default:
    epicsPrintf("Invalid data width %d\n",dmod);
    return;
  }

  dbytes=dmod/8;
  if(dmod%8)
    dbytes++;
  if(dbytes <=0 || dbytes>4){
    epicsPrintf("Invalid data width\n");
    return;
  }

  if(count<dbytes) count=dbytes;

  if(offset%dbytes!=0 || count%dbytes!=0) {
      printf("Unaligned transfers are not supported\n");
      return;
  }

  if(diaglen && offset+count > diaglen) {
      printf("Out of range\n");
      return;
  }

  count/=dbytes;

  for(i=0, dptr=diagbase+offset; i<count; i++, dptr+=dbytes) {
      if ((i*dbytes)%16==0)
          printf("\n0x%08x ",i*dbytes);
      else if ((i*dbytes)%4==0)
          printf(" ");

      switch(dmod){
      case 8:  tval=ioread8(dptr); printf("%02x",tval);break;
      case 16: tval=nat_ioread16(dptr);printf("%04x",tval);break;
      case 32: tval=nat_ioread32(dptr);printf("%08x",tval);break;
      }
  }
  printf("\n");
}

static const iocshArg pcidiagsetArg0 = { "Bus",iocshArgInt};
static const iocshArg pcidiagsetArg1 = { "Device",iocshArgInt};
static const iocshArg pcidiagsetArg2 = { "Function",iocshArgInt};
static const iocshArg pcidiagsetArg3 = { "BAR",iocshArgInt};
static const iocshArg pcidiagsetArg4 = { "PCI vendor ID",iocshArgInt};
static const iocshArg pcidiagsetArg5 = { "PCI device ID",iocshArgInt};
static const iocshArg pcidiagsetArg6 = { "exact",iocshArgInt};
static const iocshArg * const pcidiagsetArgs[7] =
    {&pcidiagsetArg0,&pcidiagsetArg1,&pcidiagsetArg2,&pcidiagsetArg3,&pcidiagsetArg4,&pcidiagsetArg5,&pcidiagsetArg6};
static const iocshFuncDef pcidiagsetFuncDef =
    {"pcidiagset",7,pcidiagsetArgs};

static void pcidiagsetCall(const iocshArgBuf *args)
{
    pcidiagset(args[0].ival, args[1].ival, args[2].ival, args[3].ival, args[4].ival, args[5].ival, args[6].ival);
}

static const iocshArg pcireadArg0 = { "data width (8,16,32)",iocshArgInt};
static const iocshArg pcireadArg1 = { "offset",iocshArgInt};
static const iocshArg pcireadArg2 = { "count",iocshArgInt};
static const iocshArg * const pcireadArgs[3] =
    {&pcireadArg0,&pcireadArg1,&pcireadArg2};
static const iocshFuncDef pcireadFuncDef =
    {"pciread",3,pcireadArgs};

static void pcireadCall(const iocshArgBuf *args)
{
    pciread(args[0].ival, args[1].ival, args[2].ival);
}

static void pcish(void)
{
    iocshRegister(&pcireadFuncDef,pcireadCall);
    iocshRegister(&pcidiagsetFuncDef,pcidiagsetCall);
}
epicsExportRegistrar(pcish);

