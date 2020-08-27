/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <stdlib.h>

#include <epicsVersion.h>
#include <epicsAssert.h>
#include <epicsTypes.h>
#include <epicsExport.h>
#include <epicsStdio.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsMMIO.h>
#include <devLibPCI.h>

static const epicsPCIDevice *diagdev;
static volatile void *diagbase;
static epicsUInt32 diaglen;

struct bdf {
    unsigned int b,d,f;
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

    diagbase = NULL;
    diagdev = NULL;
    diaglen = 0;

    printf("Looking for %x:%x.%x\n", b, d, f);

    if(vendor==0 && !exact)
        ids[0].vendor=DEVPCI_ANY_VENDOR;
    if(device==0 && !exact)
        ids[0].device=DEVPCI_ANY_DEVICE;

    loc.b=b;
    loc.d=d;
    loc.f=f;
    loc.dev=0;

    if(devPCIFindCB(ids, &matchbdf, (void*)&loc, 0)) {
        fprintf(stderr, "Error searching\n");
        return;
    }

    if(!loc.dev) {
        fprintf(stderr, "No such device\n");
        return;
    }

    printf("Mapping %x:%x.%x\n", loc.dev->bus, loc.dev->device, loc.dev->function);

#if defined(linux)
    if(devPCIBarLen(loc.dev, bar, &len)) {
        fprintf(stderr, "Failed to get BAR length\n");
        len=0;
    }
#endif

    if(devPCIToLocalAddr(loc.dev, bar, &diagbase, 0)) {
        fprintf(stderr, "Failed to map BAR\n");
        return;
    }
    diagdev = loc.dev;
    diaglen=len;

#if defined(__linux__)
    printf("BAR %d from %p for %u bytes\n",bar, (void*)diagbase, (unsigned)diaglen);
#else
    printf("BAR %d from %p\n",bar, (void*)diagbase);
#endif

}

static int check_args(int dmod, unsigned int offset, unsigned int count)
{
    switch(dmod){
    case 8:
    case 16:
    case 32:
        break;
    default:
        fprintf(stderr, "Invalid data width %d\n",dmod);
        return 1;
    }

#if defined(__linux__)
    if(offset>=diaglen || offset+count>diaglen) {
        fprintf(stderr, "Invalid offset and/or count\n");
        return 1;
    }
#endif
    return 0;
}

void pciwrite(int dmod, int offset, int value)
{
    epicsUInt32 tval = value;
    volatile char* dptr = offset + (volatile char*)diagbase;

    if(!diagbase) {
        fprintf(stderr, "Run pcidiagset first\n");
        return;
    }

    if(check_args(dmod, offset, 1))
        return;

    switch(dmod){
    case 8: iowrite8(dptr, tval); break;
    case 16: nat_iowrite16(dptr, tval); break;
    case 32: nat_iowrite32(dptr, tval); break;
    }
}

void pciread(int dmod, int offset, int count)
{
    epicsUInt32 tval;
    volatile char* dptr;
    short dbytes;
    int i;

    if(!diagbase) {
        fprintf(stderr, "Run pcidiagset first\n");
        return;
    }

    if(check_args(dmod, offset, count))
        return;

    dbytes=dmod/8;

    count/=dbytes;
    if(count==0) count=1;

    for(i=0, dptr=offset+(volatile char*)diagbase; i<count; i++, dptr+=dbytes) {
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

void pciconfread(int dmod, int offset, int count)
{
    int err = 0;
    short dbytes;

    if(!diagdev) {
        fprintf(stderr, "Run pcidiagset first\n");
        return;
    }

    if(check_args(dmod, offset, count))
        return;

    dbytes=dmod/8;

    count/=dbytes;
    if(count==0) count=1;

    for(;count && !err;offset+=dbytes,count--) {
        epicsUInt8 u8;
        epicsUInt16 u16;
        epicsUInt32 u32;
        printf("0x%04x ", offset);
        switch(dmod) {
        case 8: err = devPCIConfigRead8(diagdev, offset, &u8); printf("%02x\n", u8); break;
        case 16: err = devPCIConfigRead16(diagdev, offset, &u16); printf("%04x\n", u16); break;
        case 32: err = devPCIConfigRead32(diagdev, offset, &u32); printf("%08x\n", u32); break;
        default:
            fprintf(stderr, "Invalid dmod %d, must be 8, 16, or 32\n", dmod);
            break;
        }
    }
    if(err)
        fprintf(stderr, "read error %d\n", err);
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

static const iocshArg pciwriteArg0 = { "data width (8,16,32)",iocshArgInt};
static const iocshArg pciwriteArg1 = { "offset",iocshArgInt};
static const iocshArg pciwriteArg2 = { "value",iocshArgInt};
static const iocshArg * const pciwriteArgs[3] =
{&pciwriteArg0,&pciwriteArg1,&pciwriteArg2};
static const iocshFuncDef pciwriteFuncDef =
{"pciwrite",3,pciwriteArgs};

static void pciwriteCall(const iocshArgBuf *args)
{
    pciwrite(args[0].ival, args[1].ival, args[2].ival);
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

static const iocshArg pciconfreadArg0 = { "data width (8,16,32)",iocshArgInt};
static const iocshArg pciconfreadArg1 = { "offset",iocshArgInt};
static const iocshArg pciconfreadArg2 = { "count",iocshArgInt};
static const iocshArg * const pciconfreadArgs[3] =
{&pciconfreadArg0,&pciconfreadArg1,&pciconfreadArg2};
static const iocshFuncDef pciconfreadFuncDef =
{"pciconfread",3,pciconfreadArgs};

static void pciconfreadCall(const iocshArgBuf *args)
{
    pciconfread(args[0].ival, args[1].ival, args[2].ival);
}

static void pcish(void)
{
    iocshRegister(&pciwriteFuncDef,pciwriteCall);
    iocshRegister(&pcireadFuncDef,pcireadCall);
    iocshRegister(&pciconfreadFuncDef,pciconfreadCall);
    iocshRegister(&pcidiagsetFuncDef,pcidiagsetCall);
}
epicsExportRegistrar(pcish);

