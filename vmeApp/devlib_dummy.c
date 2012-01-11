/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <devLib.h>

/*
 * Before 3.14.9 there was not default implementation of devLib
 * so host builds were not possible.
 *
 * add a null implementation
 */

const char *epicsAddressTypeName[] = {"","","","","",""};

long    devAddressMap(void) {return -1;}

long    devReadProbe (unsigned wordSize, volatile const void *ptr, void *pValueRead) {return -1;}

long devNoResponseProbe(
                        epicsAddressType addrType,
                        size_t base,
                        size_t size
) {return -1;}

long    devWriteProbe (unsigned wordSize, volatile void *ptr, const void *pValueWritten) {return -1;}


long    devRegisterAddress(
                        const char *pOwnerName,
                        epicsAddressType addrType,
                        size_t logicalBaseAddress,
                        size_t size, /* bytes */
                        volatile void **pPhysicalAddress) {return -1;}

long    devUnregisterAddress(
                        epicsAddressType addrType,
                        size_t logicalBaseAddress,
                        const char *pOwnerName) {return -1;}

long    devAllocAddress(
                        const char *pOwnerName,
                        epicsAddressType addrType,
                        size_t size,
                        unsigned alignment, /*n ls bits zero in addr*/
                        volatile void **pLocalAddress) {return -1;}


long devDisableInterruptLevelVME (unsigned level) {return -1;}

void *devLibA24Malloc(size_t l) {return NULL;}
void *devLibA24Calloc(size_t l) {return NULL;}
void devLibA24Free(void *pBlock) {}

long    devConnectInterrupt(
                        epicsInterruptType intType,
                        unsigned vectorNumber,
                        void (*pFunction)(void *),
                        void  *parameter) {return -1;}

long    devDisconnectInterrupt(
                        epicsInterruptType      intType,
                        unsigned                vectorNumber,
                        void                    (*pFunction)(void *)) {return -1;}

long devEnableInterruptLevel(epicsInterruptType intType, unsigned level) {return -1;}

long devDisableInterruptLevel (epicsInterruptType intType, unsigned level) {return -1;}

long locationProbe (epicsAddressType addrType, char *pLocation) {return -1;}

#ifndef vxWorks
void bcopyLongs(char *source, char *destination, int nlongs) {}
#endif

devLibVirtualOS *pdevLibVirtualOS=NULL;
