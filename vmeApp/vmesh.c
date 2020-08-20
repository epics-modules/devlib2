/*************************************************************************\
* Copyright (c) 2015 Michael Davidsaver
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <stdlib.h>
#include <string.h>

#include <epicsVersion.h>
#include <epicsAssert.h>
#include <epicsTypes.h>
#include <epicsInterrupt.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsStdio.h>
#include <epicsExport.h>
#include <devcsr.h>

static
int validate_widths(epicsUInt32 addr, int amod, int dmod, int count, volatile void** mptr)
{
  epicsAddressType atype;
  short dbytes;
  epicsUInt32 alim;

  switch(dmod){
  case 8:
  case 16:
  case 32:
      break;
  default:
    fprintf(stderr, "Invalid data width %d\n",dmod);
    return 1;
  }

  switch(amod){
  case 16: atype=atVMEA16; alim = 0xffff; break;
  case 24: atype=atVMEA24; alim = 0xffffff; break;
  case 32: atype=atVMEA32; alim = 0xffffffff; break;
  default:
    fprintf(stderr, "Invalid address width %d\n",amod);
    return 1;
  }

  dbytes=dmod/8;
  if(dmod%8)
    dbytes++;
  if(dbytes <=0 || dbytes>4){
    fprintf(stderr, "Invalid data width\n");
    return 1;
  }

  if( (addr > alim) ||
      (addr+count*dbytes >= alim)) {
      fprintf(stderr, "Address/count out of range\n");
      return 1;
  }

  if( devBusToLocalAddr(
    atype, addr, mptr
  ) ){
    fprintf(stderr, "Invalid register address\n");
    return 1;
  }

  printf("Mapped to %p\n",*mptr);

  return 0;
}

/** Read some consecutive VME addresses and print to screen
 *
 * @param rawaddr The first VME address to read
 * @param amod Address width either 8, 16, or 32
 * @param dmod Data width either 8, 16, or 32
 * @param count Number of read operations
 */
void vmeread(int rawaddr, int amod, int dmod, int count)
{
  epicsUInt32 addr = rawaddr;
  volatile void* mptr;
  volatile char* dptr;
  short dbytes;
  int i, berr = 0;

  if(count<1) count=1;

  printf("Reading from 0x%08x A%d D%d\n",addr,amod,dmod);

  if(validate_widths(addr, amod, dmod, count, &mptr))
    return;

  dbytes=dmod/8;

  for(i=0, dptr=mptr; i<count; i++, dptr+=dbytes) {
      epicsUInt32 tval = -1;
      if ((i*dbytes)%16==0)
          printf("\n0x%08x ",i*dbytes);
      else if ((i*dbytes)%4==0)
          printf(" ");

      berr |= devReadProbe(dbytes, dptr, &tval);

      switch(dmod){
      case 8:  printf("%02x",(tval>>24)&0xff);break;
      case 16: printf("%04x",(tval>>16)&0xffff);break;
      case 32: printf("%08x",tval&0xffffffff);break;
      default: break; /* excluded by validate_widths() */
      }
  }
  printf("\n");
  if(berr)
      fprintf(stderr, "*** Bus errors occurred ***\n");
}

static const iocshArg vmereadArg0 = { "address",iocshArgInt};
static const iocshArg vmereadArg1 = { "amod",iocshArgInt};
static const iocshArg vmereadArg2 = { "dmod",iocshArgInt};
static const iocshArg vmereadArg3 = { "count",iocshArgInt};
static const iocshArg * const vmereadArgs[4] =
    {&vmereadArg0,&vmereadArg1,&vmereadArg2,&vmereadArg3};
static const iocshFuncDef vmereadFuncDef =
    {"vmeread",4,vmereadArgs};

static void vmereadCall(const iocshArgBuf *args)
{
    vmeread(args[0].ival, args[1].ival, args[2].ival, args[3].ival);
}

/** Write an integer value to a VME address
 *
 * @param rawaddr The first VME address to read
 * @param amod Address width either 8, 16, or 32
 * @param dmod Data width either 8, 16, or 32
 * @param rawvalue The value to write
 */
void vmewrite(int rawaddr, int amod, int dmod, int rawvalue)
{
  epicsUInt32 addr = rawaddr, value = rawvalue;
  volatile void* mptr;
  long err;

  printf("Writing to 0x%08x A%d D%d value 0x%08x\n",addr,amod,dmod, (unsigned)value);

  if(validate_widths(addr, amod, dmod, 1, &mptr))
    return;

  switch (dmod) {
  case 8: value<<=24; break;
  case 16: value<<=16; break;
  case 32: break;
  default: break; /* excluded by validate_widths() */
  }

  err = devWriteProbe(dmod/8, mptr, &value);

  if(err)
      fprintf(stderr, "*** Bus Error detected ***\n");
}

static const iocshArg vmewriteArg0 = { "address",iocshArgInt};
static const iocshArg vmewriteArg1 = { "amod",iocshArgInt};
static const iocshArg vmewriteArg2 = { "dmod",iocshArgInt};
static const iocshArg vmewriteArg3 = { "count",iocshArgInt};
static const iocshArg * const vmewriteArgs[4] =
    {&vmewriteArg0,&vmewriteArg1,&vmewriteArg2,&vmewriteArg3};
static const iocshFuncDef vmewriteFuncDef =
    {"vmewrite",4,vmewriteArgs};

static void vmewriteCall(const iocshArgBuf *args)
{
    vmewrite(args[0].ival, args[1].ival, args[2].ival, args[3].ival);
}

static volatile epicsUInt8 vmeautodisable[256];

static const char hexchars[] = "0123456789ABCDEF";

static
void vmesh_handler(void *raw)
{
  volatile epicsUInt8 *ent = raw;
  unsigned char vect = (unsigned char)(ent-vmeautodisable);
  char msg[] = "VME IRQ on vector 0xXY\n";
  unsigned I = sizeof(msg)-3;

  msg[I--] = hexchars[vect&0xf];
  vect>>=4;
  msg[I--] = hexchars[vect&0xf];
  epicsInterruptContextMessage(msg);

  if(*ent && devDisableInterruptLevelVME(*ent))
    epicsInterruptContextMessage("oops, can't disable level");
}

/** Attach a dummy interrupt handler which prints a message to screen
 *
 * @param level The interrupt level (1-7)
 * @param vector The vector code (0-255)
 * @param itype Either "rora" or "roak" acknowledgement type
 *
 * In "roak" (Release On AcKnowledge) the interrupt level
 * is left active when the interrupt occurs.
 * In "rora" (Release On Register Access) the interrupt level
 * is disabled each time the given vector is interrupted.
 * For "rora" vmeirq() should be called after each interrupt to
 * re-enabled the level.
 */
void vmeirqattach(int level, int vector, const char *itype)
{
  int acktype;
  if(strcmp(itype, "rora")==0) {
    acktype = 1;
  } else if(strcmp(itype, "roak")==0) {
    acktype = 0;
  } else {
    fprintf(stderr, "Unknown IRQ ack method '%s' (must be \"rora\" or \"roak\")\n", itype);
    return;
  }
  if(level<1 || level>7) {
    fprintf(stderr, "IRQ level %d out of range (1-7)\n", level);
    return;
  }
  if(vector>255) {
    fprintf(stderr, "IRQ vector %d out of range (1-7)\n", vector);
    return;
  }
  if(vmeautodisable[vector]) {
    fprintf(stderr, "Vector already in use\n");
    return;
  }
  if(acktype)
    iowrite8(&vmeautodisable[vector], level);
  if(devConnectInterruptVME(vector, &vmesh_handler, (void*)&vmeautodisable[vector]))
    fprintf(stderr, "Failed to install ISR\n");
}

static const iocshArg vmeirqattachArg0 = { "level",iocshArgInt};
static const iocshArg vmeirqattachArg1 = { "vector",iocshArgInt};
static const iocshArg vmeirqattachArg2 = { "acktype",iocshArgString};
static const iocshArg * const vmeirqattachArgs[3] =
    {&vmeirqattachArg0,&vmeirqattachArg1,&vmeirqattachArg2};
static const iocshFuncDef vmeirqattachFuncDef =
    {"vmeirqattach",3,vmeirqattachArgs};

static void vmeirqattachCall(const iocshArgBuf *args)
{
    vmeirqattach(args[0].ival, args[1].ival, args[2].sval);
}

/** En/Disable a VME interrupt level
 *
 * @param level The interrupt level (1-7)
 * @param act 0 - disable, 1 - enable the given level.
 */
void vmeirq(int level, int act)
{
  if(level<1 || level>7) {
    fprintf(stderr, "IRQ level %d out of range (1-7)\n", level);
    return;
  }
  if(act) {
    if(devEnableInterruptLevelVME(level))
      fprintf(stderr, "Failed to enable level\n");
  } else {
    if(devDisableInterruptLevelVME(level))
      fprintf(stderr, "Failed to disable level\n");
  }
}

static const iocshArg vmeirqArg0 = { "level",iocshArgInt};
static const iocshArg vmeirqArg1 = { "en/disable",iocshArgInt};
static const iocshArg * const vmeirqArgs[2] =
    {&vmeirqArg0,&vmeirqArg1};
static const iocshFuncDef vmeirqFuncDef =
    {"vmeirq",2,vmeirqArgs};

static void vmeirqCall(const iocshArgBuf *args)
{
    vmeirq(args[0].ival, args[1].ival);
}

static void vmesh(void)
{
    iocshRegister(&vmereadFuncDef,vmereadCall);
    iocshRegister(&vmewriteFuncDef,vmewriteCall);
    iocshRegister(&vmeirqattachFuncDef,vmeirqattachCall);
    iocshRegister(&vmeirqFuncDef,vmeirqCall);
#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<10
    devReplaceVirtualOS();
#endif
}
epicsExportRegistrar(vmesh);
