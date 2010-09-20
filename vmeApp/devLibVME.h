/*************************************************************************\
* Copyright (c) 2008 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * Compatibility overlay for devLib
 *
 * Provide updated interface (VME CSR).
 * This is a stripped down version of devLib.h having
 * only the interface functions which use the epicsAddressType
 * enum (externally or internally).
 *
 */

#ifndef INCdevLibVMEh
#define INCdevLibVMEh 1

#include "dbDefs.h"
#include "osdVME.h"
#include "errMdef.h"
#include "shareLib.h"
#include "devLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Earlier versions of EPICS may not implement CR/CSR addressing
 * in base devLib.  Make sure that we have a definition for atVMECSR
 * and that atLast indicates the end of the devLib2 address tables.
 */

#define atVMECSR ((epicsAddressType)4)
#define atLast   ((epicsAddressType)5)

/*
 * pointer to an array of strings for each of
 * the VME address types
 */
epicsShareExtern const char *epicsAddressTypeName2[];
#define epicsAddressTypeName epicsAddressTypeName2

/*
 *  General API
 *
 *  This section applies to all bus types
 */

epicsShareFunc long devAddressMap2(void); /* print an address map */
#define devAddressMap devAddressMap2

/*
 * devBusToLocalAddr()
 *
 * OSI routine to translate bus addresses their local CPU address mapping
 */
epicsShareFunc long devBusToLocalAddr2 (
		epicsAddressType addrType,
		size_t busAddr,
		volatile void **ppLocalAddr);
#define devBusToLocalAddr devBusToLocalAddr2

epicsShareFunc long devRegisterAddress2(
			const char *pOwnerName,
			epicsAddressType addrType,
			size_t logicalBaseAddress,
			size_t size, /* bytes */
			volatile void **pPhysicalAddress);
#define devRegisterAddress devRegisterAddress2

epicsShareFunc long devUnregisterAddress2(
			epicsAddressType addrType,
			size_t logicalBaseAddress,
			const char *pOwnerName);
#define devUnregisterAddress devUnregisterAddress2

/*
 * allocate and register an unoccupied address block
 */
epicsShareFunc long devAllocAddress2(
			const char *pOwnerName,
			epicsAddressType addrType,
			size_t size,
			unsigned alignment, /*n ls bits zero in addr*/
			volatile void **pLocalAddress);
#define devAllocAddress devAllocAddress2

extern devLibVirtualOS *pdevLibVME2;
epicsShareFunc void devReplaceVirtualOS(void);

#ifdef __cplusplus
}
#endif

#endif  /* INCdevLibVMEh.h*/
