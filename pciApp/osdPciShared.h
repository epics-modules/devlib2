/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef OSDPCISHARED_H_INC
#define OSDPCISHARED_H_INC

#include "devLibPCIOSD.h"

#include <dbDefs.h>
#include <shareLib.h>

/* Subtract member byte offset, returning pointer to parent object
 *
 * Added in Base 3.14.11
 */
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

struct osdPCIDevice {
  epicsPCIDevice dev; /* "public" data */

  /* Can be used to cache values */
  /* (PCI bus addresses are not program space pointers!) */
  epicsUInt32 base[PCIBARCOUNT];
  epicsUInt32 len[PCIBARCOUNT];
  epicsUInt32 erom;

  ELLNODE node;

  void *drvpvt; /* for out of tree drivers */
};
typedef struct osdPCIDevice osdPCIDevice;

INLINE
osdPCIDevice*
pcidev2osd(const epicsPCIDevice *devptr)
{
    return CONTAINER((epicsPCIDevice*)devptr,osdPCIDevice,dev);
}

int
sharedDevPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
);

int
sharedDevPCIToLocalAddr(
  const epicsPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
);

int
sharedDevPCIBarLen(
  const epicsPCIDevice* dev,
  unsigned int bar,
  epicsUInt32 *len
);

int
sharedDevPCIConfigAccess(
  const epicsPCIDevice *dev,
  unsigned offset,
  void *pArg,
  devPCIAccessMode mode
);

int sharedDevPCIInit(void);

#endif /* OSDPCISHARED_H_INC */
