
#ifndef DEVLIBPCIOSD_H_INC
#define DEVLIBPCIOSD_H_INC

#include <epicsMutex.h>

#include "devLibPCIImpl.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct osdPCIDevice
{
  epicsPCIDevice dev; /* "public" data */

  /* result of mmap(), add offset before passing to user */
  volatile void* base[PCIBARCOUNT];
  /* offset from start of page to start of BAR */
  epicsUInt32 offset[PCIBARCOUNT];
  /* BAR length (w/o offset) */
  epicsUInt32 len[PCIBARCOUNT];
  volatile void* erom;
  epicsUInt32 eromlen;

  epicsUInt32 displayBAR[PCIBARCOUNT]; /* Raw PCI address */
  epicsUInt32 displayErom;

  int fd;  /* /dev/uio# */
  int cfd; /* config-space descriptor */
  int rfd[PCIBARCOUNT];
  int cmode;            /* config-space mode */

  epicsMutexId devLock; /* guard access to isrs list */

  /* Optional callback invoked on PCI device hot-swap.*/
  void (*onHotSwapHook)(struct osdPCIDevice*);

  ELLNODE node;

  ELLLIST isrs; /* contains struct osdISR */
};
typedef struct osdPCIDevice osdPCIDevice;

#ifdef __cplusplus
}
#endif

#endif /* DEVLIBPCIOSD_H_INC */
