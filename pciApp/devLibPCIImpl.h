/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef DEVLIBPCIIMPL_H_INC
#define DEVLIBPCIIMPL_H_INC

#include <stddef.h>

#include <dbDefs.h>
#include <ellLib.h>
#include <shareLib.h>
#include <epicsTypes.h>

#include "devLibPCI.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RD_08 = 0x01,
    RD_16 = 0x02,
    RD_32 = 0x04,
    WR_08 = 0x11,
    WR_16 = 0x12,
    WR_32 = 0x14
} devPCIAccessMode;

#define CFG_ACC_WIDTH(mode) ((mode) & 0x0f)
#define CFG_ACC_WRITE(mode) ((mode) & 0x10)

typedef struct {
  const char *name;

  int (*pDevInit)(void);

  int (*pDevFinal)(void);

  int (*pDevPCIFind)(const epicsPCIID *ids, devPCISearchFn searchfn, void *arg, unsigned int o);

  int (*pDevPCIToLocalAddr)(const epicsPCIDevice* dev,unsigned int bar,volatile void **a,unsigned int o);

  int (*pDevPCIBarLen)(const epicsPCIDevice* dev,unsigned int bar,epicsUInt32 *len);

  int (*pDevPCIConnectInterrupt)(const epicsPCIDevice *id,
                                 void (*pFunction)(void *),
                                 void  *parameter,
                                 unsigned int opt);

  int (*pDevPCIDisconnectInterrupt)(const epicsPCIDevice *id,
                                    void (*pFunction)(void *),
                                    void  *parameter);

  int (*pDevPCIConfigAccess)(const epicsPCIDevice *id, unsigned offset, void *pArg, devPCIAccessMode mode);

  /* level 0 enables, higher levels disable - on error a negative value is returned */
  int (*pDevPCISwitchInterrupt)(const epicsPCIDevice *id, int level);
  ELLNODE node;
} devLibPCI;

epicsShareFunc
int
devLibPCIRegisterDriver2(devLibPCI*, size_t);

#define devLibPCIRegisterDriver(TPTR) devLibPCIRegisterDriver2(TPTR, sizeof(*(TPTR)))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBPCIIMPL_H_INC */
