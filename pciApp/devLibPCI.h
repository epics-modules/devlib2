
#ifndef DEVLIBPCI_H_INC
#define DEVLIBPCI_H_INC 1

#include <dbDefs.h>
#include <epicsTypes.h>
#include <devLib.h>
#include <shareLib.h>

/**
 * @defgroup pci PCI Bus Access
 *
 * Library to support PCI bus access
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PCI device identifier
 *
 * This structure is used to hold identifying information for a PCI
 * device.  When used for searching each field can hold
 * a specific value or a wildcard.
 *
 * Fields are oversized to allow a
 * distinct wildcard value.
 *
 * There is a DEVPCI_ANY_* wildcard macro for each field.
 * Most will use the convienence macros defined below.
 *
 * PCI identifer lists should be defined like:
 *
 @code
 static const epicsPCIID mydevs[] = {
     DEVPCI_SUBDEVICE_SUBVENDOR( 0x1234, 0x1030, 0x0001, 0x4321 ),
     DEVPCI_SUBDEVICE_SUBVENDOR( 0x1234, 0x1030, 0x0002, 0x4321 ),
     DEVPCI_END
 };
 @endcode
 */
typedef struct {
  epicsUInt32 device, vendor;
  epicsUInt32 sub_device, sub_vendor;
  epicsUInt32 pci_class;
  epicsUInt16 revision;
} epicsPCIID;

#define DEVPCI_ANY_DEVICE 0x10000
#define DEVPCI_ANY_VENDOR 0x10000
#define DEVPCI_ANY_SUBDEVICE 0x10000
#define DEVPCI_ANY_SUBVENDOR 0x10000
#define DEVPCI_ANY_CLASS 0x1000000
#define DEVPCI_ANY_REVISION 0x100

/** @brief The last item in a list of PCI IDS */
#define DEVPCI_END {0,0,0,0,0,0}

#define DEVPCI_DEVICE_VENDOR(dev,vend) \
{ dev, vend, DEVPCI_ANY_SUBDEVICE, DEVPCI_ANY_SUBVENDOR, \
DEVPCI_ANY_CLASS, DEVPCI_ANY_REVISION }

#define DEVPCI_DEVICE_VENDOR_CLASS(dev,vend,pclass) \
{ dev, vend, DEVPCI_ANY_SUBDEVICE, DEVPCI_ANY_SUBVENDOR, \
pclass, DEVPCI_ANY_REVISION }

#define DEVPCI_SUBDEVICE_SUBVENDOR(dev,vend,sdev,svend) \
{ dev, vend, sdev, svend, \
DEVPCI_ANY_CLASS, DEVPCI_ANY_REVISION }

#define DEVPCI_SUBDEVICE_SUBVENDOR_CLASS(dev,vend,sdev,svend,revision,pclass) \
{ dev, vend, sdev, svend, \
pclass, revision }

struct PCIBar {
  unsigned int ioport:1; /** 0 memory, 1 I/O */
  unsigned int addr64:1; /** 0 32 bit, 1 64 bit */
  unsigned int below1M:1; /** 0 Normal, 1 Must be mapped below 1M */
};

/** @brief Device token
 *
 * When a PCI device is found with one of the search functions
 * a pointer to ::epicsPCIDevice instance is returned.
 * It will be passed to all subsequent API calls.
 *
 * @warning This instance must not be modified since it is shared by all callers.
 */

typedef struct {
  epicsPCIID   id; /**< @brief Exact ID of device */
  unsigned int bus;
  unsigned int device;
  unsigned int function;
  struct PCIBar bar[6];
  epicsUInt8 irq;
} epicsPCIDevice;

/** @brief The maximum number of base address registers (BARs). */
#define PCIBARCOUNT NELEMENTS( ((epicsPCIDevice*)0)->bar )

/** @brief PCI search callback prototype
 *
 @param ptr User pointer
 @param dev PCI device pointer
 */
typedef int (*devPCISearchFn)(void* ptr,epicsPCIDevice* dev);

/** @brief PCI bus search w/ callback
 *
 * Iterate through all devices in the system and invoke
 * the provided callback for all those matching the
 * provided ID list.
 *
 @param idlist List of PCI identifiers
 @param searchfn User callback
 @param arg User pointer
 @param opt Modifiers.  Currently unused
 @retval 0 Success
 @retval !0 Failure
 */
epicsShareFunc
int devPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
);

/** @brief PCI bus probe
 *
 * Probe and test a single address.  It it matches
 * the corresponding ::epicsPCIDevice instance is
 * stored in 'found'
 *
 @param idlist List of PCI identifiers
 @param b bus
 @param d device
 @param f function
 @param[out] found On success the results is stored here
 @param opt Modifiers.  Currently unused
 @retval 0 Success
 @retval !0 Failure
 */
epicsShareFunc
int devPCIFindBDF(
     const epicsPCIID *idlist,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
      epicsPCIDevice **found,
     unsigned int opt /* always 0 */
);

/** @brief Get pointer to PCI BAR
 *
 * Map a PCI BAR into the local process address space.
 *
 @param id PCI device pointer
 @param bar BAR number
 @param[out] ppLocalAddr Pointer to start of BAR
 @param opt Modifiers.  Currently unused
 @retval 0 Success
 @retval !0 Failure
 */
epicsShareFunc
int
devPCIToLocalAddr(
        epicsPCIDevice *id,
          unsigned int  bar,
        volatile void **ppLocalAddr,
           unsigned int opt /* always 0 */
);

/** @brief Find the size of a BAR
 *
 * Returns the size (in bytes) of the region visible through
 * the given BAR.
 *
 @warning On RTEMS and vxWorks this is a destructive operations.
          When calling it ensure that nothing access the device.
         \b "Don't call this on a device used by another driver."
 *
 @param id PCI device pointer
 @param bar BAR number
 @returns The BAR length or 0.
 */
epicsShareFunc
epicsUInt32
devPCIBarLen(
        epicsPCIDevice *id,
          unsigned int  bar
);

/** @brief Request interrupts for device
 *
 * Request that the provided callback be
 * invoked whenever the device asserts
 * an interrupt.
 *
 @note Always connect the interrupt handler before allowing
       the device to send.
 *
 @note All drivers should be prepared for the device to share an interrupt
       with other devices.
 *
 @param id PCI device pointer
 @param pFunction User ISR
 @param parameter User pointer
 @param opt Modifiers.  Currently unused
 */
epicsShareFunc
int devPCIConnectInterrupt(
        epicsPCIDevice *id,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt /* always 0 */
);

/** @brief Stop receiving interrupts
 *
 * Use the same arguments passed to devPCIConnectInterrupt()
 @param id PCI device pointer
 @param pFunction User ISR
 @param parameter User pointer
 */
epicsShareFunc
int devPCIDisconnectInterrupt(
        epicsPCIDevice *id,
  void (*pFunction)(void *),
  void  *parameter
);

epicsShareFunc
void
devPCIShow(int lvl, int vendor, int device, int exact);

epicsShareFunc
void
devPCIShowDevice(int lvl, epicsPCIDevice *dev);

/* Select driver implementation by name, or NULL to use default.
 * If no selection is made then the default will be used if available.
 */
epicsShareFunc
int
devLibPCIUse(const char*);

epicsShareFunc
const char* devLibPCIDriverName();

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DEVLIBPCI_H_INC */
