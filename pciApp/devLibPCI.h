/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

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

#define DEVLIBPCI_MAJOR 1 /**< @brief API major version */
#define DEVLIBPCI_MINOR 2 /**< @brief API minor version */

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

#define DEVPCI_LAST_DEVICE 0xffff0000

/** @brief The last item in a list of PCI IDS */
#define DEVPCI_END {DEVPCI_LAST_DEVICE,0,0,0,0,0}

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
  unsigned int ioport:1; /**< 0 memory, 1 I/O */
  unsigned int addr64:1; /**< 0 32 bit, 1 64 bit */
  unsigned int below1M:1; /**< 0 Normal, 1 Must be mapped below 1M */
};

/** @brief Device token
 *
 * When a PCI device is found with one of the search functions
 * a pointer to an ::epicsPCIDevice instance is returned.
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
  unsigned int domain;
  const char* driver;
} epicsPCIDevice;

/** @brief The maximum number of base address registers (BARs). */
#define PCIBARCOUNT NELEMENTS( ((epicsPCIDevice*)0)->bar )

/** @brief PCI search callback prototype
 *
 @param ptr User pointer
 @param dev PCI device pointer
 @return 0 Continue search
 @return 1 Abort search Ok (devPCIFindCB() returns 0)
 @return other Abort search failed (devPCIFindCB() returns this code)
 */
typedef int (*devPCISearchFn)(void* ptr,const epicsPCIDevice* dev);

/** @brief PCI bus search w/ callback
 *
 * Iterate through all devices in the system and invoke
 * the provided callback for those matching an entry in the
 * provided ID list.
 *
 * Iteration will stop when the callback returns a non-zero value.
 * If the callback returns 1 this call will return 0.  Any other value
 * will be returned without modification.
 *
 @param idlist List of PCI identifiers
 @param searchfn User callback
 @param arg User pointer
 @param opt Modifiers.  Currently unused
 @returns 0 on success or the error code returned by the callback.
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
 * Probe and test a single address.  If it matches,
 * the corresponding ::epicsPCIDevice instance is
 * stored in 'found'.
 *
 * If no compatible device is present the call returns
 * S_dev_noDevice.
 *
 @param idlist List of PCI identifiers
 @param domain domain
 @param b bus
 @param d device
 @param f function
 @param[out] found On success the result is stored here
 @param opt Modifiers.  Currently unused
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int devPCIFindDBDF(
     const epicsPCIID *idlist,
     unsigned int      domain,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
const epicsPCIDevice **found,
     unsigned int opt /* always 0 */
);

epicsShareFunc
int devPCIFindBDF(
     const epicsPCIID *idlist,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
const epicsPCIDevice **found,
     unsigned int opt /* always 0 */
);

#ifdef __linux__
#define DEVLIB_MAP_UIO1TO1 0
#define DEVLIB_MAP_UIOCOMPACT 1
#else
/* UIO options have no meaning for non-Linux OSs */
#define DEVLIB_MAP_UIO1TO1 0
#define DEVLIB_MAP_UIOCOMPACT 0
#endif

/** @brief Get pointer to PCI BAR
 *
 * Map a PCI BAR into the local process address space.
 *
 * The opt argument is used to modify the mapping process.
 * Currently only two (mutually exclusive) flags are supported which are only
 * used by the Linux UIO bus implementation to control
 * how requested BAR #s are mapped to UIO region numbers.
 *
 * @li DEVLIB_MAP_UIO1TO1 (the only choice in devLib2 versions < 2.4) passes BAR
 *     #s without modification.
 * @li DEVLIB_MAP_UIOCOMPACT Maps the requested BAR # to the index of the appropriate
 *     IOMEM region.  This index skips I/O Port BARs and any other non-IOMEM regions.
 *
 @param id PCI device pointer
 @param bar BAR number
 @param[out] ppLocalAddr Pointer to start of BAR
 @param opt Modifiers.  0 or bitwise OR of one or more DEVLIB_MAP_* macros
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int
devPCIToLocalAddr(
  const epicsPCIDevice *id,
          unsigned int  bar,
        volatile void **ppLocalAddr,
           unsigned int opt /* always 0 */
);

/** @brief Find the size of a BAR
 *
 * Returns the size (in bytes) of the region visible through
 * the given BAR.
 *
 @warning On RTEMS and vxWorks this is a invasive operation.
          When calling it ensure that nothing is accessing the device.
         \b Don't \b call \b this \b on \b a \b device \b used \b by \b another \b driver.
 *
 @param id PCI device pointer
 @param bar BAR number
 @param[out] len BAR size in bytes
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int
devPCIBarLen(
  const epicsPCIDevice *id,
          unsigned int  bar,
           epicsUInt32 *len
);

/** @brief Request interrupts for device
 *
 * Request that the provided callback be
 * invoked whenever the device asserts
 * an interrupt.
 *
 @note Always connect the interrupt handler before enabling
       the device to send interrupts.
 *
 @note All drivers should be prepared for their device to share an interrupt
       with other devices.
 *
 @param id PCI device pointer
 @param pFunction User ISR
 @param parameter User pointer
 @param opt Modifiers.  Currently unused
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int devPCIConnectInterrupt(
  const epicsPCIDevice *id,
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
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int devPCIDisconnectInterrupt(
  const epicsPCIDevice *id,
  void (*pFunction)(void *),
  void  *parameter
);

epicsShareFunc
void
devPCIShow(int lvl, int vendor, int device, int exact);

epicsShareFunc
void
devPCIShowDevice(int lvl, const epicsPCIDevice *dev);

/** @brief Select driver implementation.
 * Pick driver implementation by name, or NULL to use default.
 * If no selection is made then the default will be used if available.
 *
 @param name An implementation name
 @returns 0 on success or an EPICS error code on failure.
 */
epicsShareFunc
int
devLibPCIUse(const char* name);

epicsShareFunc
const char* devLibPCIDriverName();

extern int devPCIDebug;

/** @brief Read byte from configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space
 @param   pResult Pointer to where result is to be written
 @returns 0       on success or an EPICS error code on failure
 */

epicsShareFunc
int devPCIConfigRead8(const epicsPCIDevice *dev, unsigned offset, epicsUInt8 *pResult);

/** @brief Read (16-bit) word from configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space (must be 16-bit aligned)
 @param   pResult Pointer to where result is to be written
 @returns 0       on success or an EPICS error code on failure
 */


epicsShareFunc
int devPCIConfigRead16(const epicsPCIDevice *dev, unsigned offset, epicsUInt16 *pResult);

/** @brief Read (32-bit) dword from configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space (must be 32-bit aligned)
 @param   pResult Pointer to where result is to be written
 @returns 0       on success or an EPICS error code on failure
 */

epicsShareFunc
int devPCIConfigRead32(const epicsPCIDevice *dev, unsigned offset, epicsUInt32 *pResult);

/** @brief Write byte to configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space
 @param   value   Value to be written
 @returns 0       on success or an EPICS error code on failure
 */

epicsShareFunc
int devPCIConfigWrite8(const epicsPCIDevice *dev, unsigned offset, epicsUInt8 value);

/** @brief Write (16-bit) word from configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space (must be 16-bit aligned)
 @param   value   Value to be written
 @returns 0       on success or an EPICS error code on failure
 */


epicsShareFunc
int devPCIConfigWrite16(const epicsPCIDevice *dev, unsigned offset, epicsUInt16 value);

/** @brief Write (32-bit) dword from configuration space
 *
 @param   dev     A PCI device handle
 @param   offset  Offset into configuration space (must be 32-bit aligned)
 @param   value   Value to be written
 @returns 0       on success or an EPICS error code on failure
 */

epicsShareFunc
int devPCIConfigWrite32(const epicsPCIDevice *dev, unsigned offset, epicsUInt32 value);

/** @brief Enable interrupts at the device.
 *
 @param   dev     A PCI device handle
 @returns 0       on success or an EPICS error code on failure

 @note            Implementation of this call for any OS is optional
 */
epicsShareFunc
int devPCIEnableInterrupt(const epicsPCIDevice *dev);

/** @brief Enable interrupts at the device.
 *
 @param   dev     A PCI device handle
 @returns 0       on success or an EPICS error code on failure

 @note            Implementation of this call for any OS is optional
 */

epicsShareFunc
int devPCIDisableInterrupt(const epicsPCIDevice *dev);

/** @brief Translate class id to string.
 *
 @param   classId    PCI class Id
 @returns            constant class name string
 */

epicsShareFunc
const char* devPCIDeviceClassToString(int classId);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DEVLIBPCI_H_INC */
