
#include <stdlib.h>

#include <epicsAssert.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <errlog.h>
#include <iocsh.h>
#include <devLib.h>

void vmeread(epicsUInt32 addr, int amod, int dmod)
{
  epicsUInt32 tval;
  epicsAddressType atype;
  volatile void* mptr;
  short dbytes;

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

  epicsPrintf("Reading from 0x%08x A%d D%d\n",addr,amod,dmod);

  if( devBusToLocalAddr(
    atype, addr, &mptr
  ) ){
    epicsPrintf("Invalid register address\n");
    return;
  }

  epicsPrintf("Mapped to 0x%08x for %d bytes\n",(unsigned int)mptr,dbytes);

  if( devReadProbe(
    dbytes,
    mptr,
    &tval
  ) ){
    epicsPrintf("Test read failed\n");
    return;
  }

  epicsPrintf("Read 0x%08x\n",tval);
}

/* callbackSetQueueSize */
static const iocshArg vmereadArg0 = { "address",iocshArgInt};
static const iocshArg vmereadArg1 = { "amod",iocshArgInt};
static const iocshArg vmereadArg2 = { "dmod",iocshArgInt};
static const iocshArg * const vmereadArgs[3] =
    {&vmereadArg0,&vmereadArg1,&vmereadArg2};
static const iocshFuncDef vmereadeFuncDef =
    {"vmeread",3,vmereadArgs};

void vmereadCall(const iocshArgBuf *args)
{
    vmeread(args[0].ival, args[1].ival, args[2].ival);
}

void vmesh(void)
{
    iocshRegister(&vmereadeFuncDef,vmereadCall);
}
epicsExportRegistrar(vmesh);
