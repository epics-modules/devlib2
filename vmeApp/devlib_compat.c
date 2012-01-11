/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>

#include <epicsVersion.h>
#include <devLib.h>

#define epicsExportSharedSymbols
#include <shareLib.h>

#if defined(__rtems__)
#  if !defined(__PPC__) && !defined(__mcf528x__)
#    define NEED_IFACE
#    define NEED_PIMPL
#  endif
#elif defined(vxWorks)
 /* nothing needed */
#else
#  define NEED_IFACE
#endif

#ifdef NEED_IFACE

#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<12
/*
 * Most devlib function go through an indirection table with a null
 * implimentation provided for systems which doen't impliment some
 * functionality.  However, the functions below don't use this table.
 *
 * For most functions we can use the deprecated API, but before 3.14.12
 * no wrapper for devInterruptInUseVME() was provided so can't implement
 * that one
 */

epicsShareFunc long devEnableInterruptLevelVME (unsigned vectorNumber)
{
    return devEnableInterruptLevel(intVME, vectorNumber);
}

epicsShareFunc long devConnectInterruptVME (
                        unsigned vectorNumber,
                        void (*pFunction)(void *),
                        void  *parameter)
{
    return devConnectInterrupt(intVME,vectorNumber, pFunction, parameter);
}

epicsShareFunc long devDisconnectInterruptVME (
			unsigned vectorNumber,
			void (*pFunction)(void *))
{
  return devDisconnectInterrupt(intVME, vectorNumber, pFunction);
}

epicsShareFunc int  devInterruptInUseVME (unsigned vectorNumber)
{
   return -1; /* Not implemented in Base <= 3.14.11 */
}
#endif /* EPICS < 3.14.12 */


#endif /* NEED_IFACE */

#ifdef NEED_PIMPL

#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<12
devLibVirtualOS *pdevLibVirtualOS = NULL;
#else
devLibVME *pdevLibVirtualOS = NULL;
#endif

#endif /* NEED_PIMPL */

#if !(EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<10)
#include <epicsExport.h>

void devReplaceVirtualOS(void)
{
    /* not needed after 3.14.9 */
}

epicsExportRegistrar(devReplaceVirtualOS);
#endif
