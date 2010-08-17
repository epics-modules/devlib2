
#include <stdlib.h>

#include <devLib.h>

#define epicsExportSharedSymbols
#include <shareLib.h>

#ifdef __rtems__
#  if !defined(__PPC__) && !defined(__mcf528x__)
#    define NEED_IFACE
#    define NEED_PIMPL
#  endif
#else
#  define NEED_IFACE
#endif

#ifdef NEED_IFACE

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

#endif /* NEED_IFACE */

#ifdef NEED_PIMPL

devLibVirtualOS *pdevLibVirtualOS = NULL;

#endif /* NEED_PIMPL */
