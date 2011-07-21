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
#include <devcsr.h>

void vmeread(epicsUInt32 addr, int amod, int dmod, int count)
{
  epicsUInt32 tval;
  epicsAddressType atype;
  volatile void* mptr;
  volatile char* dptr;
  short dbytes;
  int i;

  if(count<1) count=1;

  switch(dmod){
  case 8:
  case 16:
  case 32:
      break;
  default:
    epicsPrintf("Invalid data width %d\n",dmod);
    return;
  }

  switch(amod){
  case 16: atype=atVMEA16; break;
  case 24: atype=atVMEA24; break;
  case 32: atype=atVMEA32; break;
  default:
    epicsPrintf("Invalid address width %d\n",amod);
    return;
  }

  dbytes=dmod/8;
  if(dmod%8)
    dbytes++;
  if(dbytes <=0 || dbytes>4){
    epicsPrintf("Invalid data width\n");
    return;
  }

  if( (addr > ((1<<amod)-1)) ||
      (addr+count*dbytes >= ((1<<amod)-1))) {
      epicsPrintf("Address/count out of range\n");
      return;
  }

  epicsPrintf("Reading from 0x%08x A%d D%d\n",addr,amod,dmod);

  if( devBusToLocalAddr(
    atype, addr, &mptr
  ) ){
    epicsPrintf("Invalid register address\n");
    return;
  }

  epicsPrintf("Mapped to 0x%08lx for %d bytes\n",(unsigned long)mptr,dbytes*count);

  if( devReadProbe(
    dbytes,
    mptr,
    &tval
  ) ){
    epicsPrintf("Test read failed\n");
    return;
  }

  for(i=0, dptr=mptr; i<count; i++, dptr+=dbytes) {
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

static const iocshArg vmereadArg0 = { "address",iocshArgInt};
static const iocshArg vmereadArg1 = { "amod",iocshArgInt};
static const iocshArg vmereadArg2 = { "dmod",iocshArgInt};
static const iocshArg vmereadArg3 = { "count",iocshArgInt};
static const iocshArg * const vmereadArgs[4] =
    {&vmereadArg0,&vmereadArg1,&vmereadArg2,&vmereadArg3};
static const iocshFuncDef vmereadeFuncDef =
    {"vmeread",4,vmereadArgs};

static void vmereadCall(const iocshArgBuf *args)
{
    vmeread(args[0].ival, args[1].ival, args[2].ival, args[3].ival);
}

static void vmesh(void)
{
    iocshRegister(&vmereadeFuncDef,vmereadCall);
#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<10
    devReplaceVirtualOS();
#endif
}
epicsExportRegistrar(vmesh);
