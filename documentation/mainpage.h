#ifndef MAINPAGE_H
#define MAINPAGE_H

/**

@mainpage devLib2 MMIO Bus Access Library

@section whatisit What is it?

devLib2 is an extension to the EPICS OS independent VME bus access
library  found in the EPICS Base.
The @ref mmio "MMIO API" is included in EPICS Base >=3.15.0.2

@section whereis Source

Releases can be found at @url http://sourceforge.net/projects/epics/files/devlib2/

VCS source browser
@url https://github.com/epics-modules/devlib2

Or checked out with

git clone https://github.com/epics-modules/devlib2.git

The canonical version of this page is @url http://epics.sourceforge.net/devlib2/

@subsection requires Requires

EPICS Base >= 3.14.8.2

@url http://www.aps.anl.gov/epics/

@section features Features

devLib2 adds features in several areas: PCI bus access, VME64x CSR/CSR,
and memory mapped I/O (MMIO) operations.

@subsection pcisec PCI Bus

The PCI bus access interface is entirely new.  It is currently implemented for
RTEMS, vxWorks, and Linux (with @ref pcilinux "some limitations").
The PCI interface provides functions for: searching the bus, mapping devices
into process memory, and (dis)connecting interrupts.

Runtime selection of implementations is also supported.
This allows code using this interface to compile and link for all target,
even those which lack an implementation.

@li @ref pci "API Documentation"

@li @subpage pciusage

@li @ref exploreapp

@li @ref iocshpci

@subsubsection explorepci Generic PCI driver

The exploreApp @ref exploreapp is a generic EPICS driver intended to support
development of custom PCI/PCIe devices.
It provides a set of records which read/write to individual registers.
Basic support for interrupts is also available (Linux only).

@subsection vmesec VME CSR

The VME64x library provides several functions for accessing the CSR/CR
address space defined by VME64 and extended by VME64x.
This includes: probing by slot number and matching by identifier registers,
definitions of standard registers, and functions to access them.

@li @ref vmecsr "API Docmentation"

@li @subpage vmeusage

@li @ref vmecsrregs "CSR/CR Register Definitions"

@li @ref iocshvme

@subsection mmiosec MMIO

The MMIO library provides an OS and CPU architecture independent way to
safely access memory mapped I/O devices.  Makes guarantees of
the width and order of accesses.

@li @ref mmio "API Docmentation"

@section changelog Changelog

@subsection ver2c 2.12 (UNRELEASED)

@li Various fixes for Linux, and preparation for Windows support (Dirk Zimoch)
@li Fix redirection of iocsh functions

@subsection ver2b 2.11 (Apr 2020)

@li Set SHRLIB_VERSION
@li Fix handling of PCI mmap() on Linux (Dirk Zimoch)

@subsection ver2a 2.10 (Nov 2018)

@li Support Base 7.0.x
@li Publish module version to user makefiles
@li Minor bug fixes

@subsection ver29 2.9 (July 2017)

@li pci: change devPCIFindSpec() to parse B:D.F as hex.
         This is an incompatible change!
@li pci: Fixups for vxWorks 5 (Dirk Zimoch)
@li pci: Add missing offset bounds check to PCI iocsh functions
@li vme: Various fixes for VME iocsh functions
@li explore: Add exploreApp toolkit for PCI driver/hardware development (@ref exploreapp)
@li linux: Add pci_generic_msi.c UIO driver to support devices with single vector MSI

@subsection ver28 2.8 (Sept. 2016)

@li Fixes an bug with epicsMMIO.h for some targets where a single read
    macro expands to either 2 (16 bit) or 4 (32 bit) loads in situations
    where a byte swap is necessary.
    Effects all targets except RTEMS on powerpc to some degree.
    On Linux be_*() or le_*() are effected (depending on target byte order).
    On vxWorks nat_*() and le_* are effected on big endian targets,
    and le_*() on little endian targets.
@li pci: Bump API version to 1.3
@li pci: Add PCI ID match macro DEVPCI_DEVICE_ANY()
@li pci: Add member epicsPCIDevice::slot and macro DEVPCI_NO_SLOT
@li pci/linux: Look at /sys/bus/pci/slots/*/address to populate slot member.
@li pci: Add function devPCIFindSpec() to search by text string.
    Understands PCI bus address, slot number, and/or instance number
@li Add devPCIShowMatch() variant of devPCIShow() using devPCIFindSpec().

@subsection ver27 2.7 (Jan. 2016)

@li configure: optionally include \$(TOP)/configure/RELEASE.local and \$(TOP)/../RELEASE.local.
    \$(EPICS_BASE) is no longer defined by default in configure/RELEASE and must
    be explicitly set in one of the possible RELEASE* files.
@li Fix compile failure on vxWorks (Eric Bjorklund)
@li Add missing extern "C" in epicsMMIO.h for vxWorks (Eric Bjorklund)
@li In epicsMMIODef.h replace 'inline' with 'static inline' for C compatibility.
@li RTEMS: handle BSPs with offset PCI addresses
@li pci/linux: devPCIDebug>1 enables more debug output when searching/matching PCI devices.
@li pci/linux: fix error preventing use of uio devices other than uio0.
@li vme: add vmewrite(), vmeirqattach(), and vmeirq() iocsh commands
    for debugging/development with VME devices.

@subsection ver26 2.6 (May 2015)

@li VCS repository moved to github.com
@li Increment API version to 1.2 (was 1.0)
@li epicsMMIO.h included in EPICS Base 3.15.1, not installed by this module.

Work by Andrew Johnson

@li Import support for vxWorks 5.5 on m68k and powerpc from EPICS Base.

Work by Till Straumann

@li Add PCI Config space access functions devPCIConfigRead##() and devPCIConfigWrite##()
@li Add devPCIEnableInterrupt() and devPCIDisableInterrupt().
    - Linux: invoke the UIO irqcontrol callback function with a 1 or 0.
    - vxWorks: call intEnable or intDisable (enabled by default). (Dirk Zimoch)

Work by Dirk Zimoch

@li Rework of the Linux PCI bus scan to support PCI domains.
    Previously domain 0 was used implicitly.
@li Rework of the vxWorks/RTEMS PCI bus scan to allow wildcards in device search.
    This allows devPCIShow for all PCI devices like in Linux.
@li Fixes for 64 bit BARs.
@li vxWorks: use BAR address directly if sysBusToLocalAdrs does not support PCI.
@li Changes in devPCIShow to get nicer output.

Work by Michael Davidsaver

@li Add the pciconfread() iocsh function.
@li devLibPCIRegisterDriver()  is now a macro wrapping devLibPCIRegisterDriver2()
    which performs a consistency check on the size of the devLibPCI structure.
@li provide bswap16() and bswap32() for RTEMS PPC targets.  Previously omitted.
@li Change name format of Linux user "ISR" thread to include PCI BDF.


@subsection ver25 2.5 (May 2014)

@li Linux: possible string corruption in vallocPrintf()
      Found by Till Straumann
@li vxWorks: lookup symbols including sysBusToLocalAdrs at runtime.
      Work by Eric and Dirk Zimoch
@li RTEMS: Select no-op MMIO implementation for m68k (no special handling required)

@subsection ver24 2.4 (Oct. 2012)

@li Remove C++ style comments from epicsMMIODef.h and devcsr.h
@li Fix Issue preventing Custom PCI bus implementation. (Found by Dan Eichel)
@li Linux "ISR" thread stack size not correct (Fixed by Till Straumann)
@li Linux: Previous versions expected Linux kernel modules to leave gaps for un-mappable PCI BARs (I/O Ports).
    However, most existing kernel modules don't do this.  Add a flag to devPCIToLocalAddr() to specify
    how a device's kernel module does mapping.  The default is to map as in previous versions.
    (New behavior by Till Straumann)
    See documentation of devPCIToLocalAddr().

@subsection ver23 2.3 (Apr. 2012)

@li Include proper headers to enable memory barriers for vxWorks >= 6.6
@li Fallback to noop when vxWorks memory barriers are not available
@li Fix incorrect return code when connecting pci interrupts on vxWorks

@subsection ver22 2.2 (Sept. 2011)

@li linux: follow changes to /sys (needed for kernel >2.6.26)
@li pci: IOC shell functions added: pciread()
@li pci: vxworks translate PCI addresses
@li Build on vxWorks 6.8  (Thanks to Andrew Johnson)
@li pci: Make operations reentrant on RTEMS and vxWorks (previously was not oops.)
@li pci: Fix bug with devPCIBarLen on RTEMS and vxWorks.

@subsection ver21 2.1 (Jan. 2011)

@li Fix build issue with 3.14.12 with RTEMS pc386 (found by Jim Chen from Hytec)
@li Add notification of missed PCI interrupt events on Linux
@li Additional arguement 'count' for vmeread() to show blocks of registers
@li Add section to PCI Usage on x86 Port I/O
@li Add section to PCI Access in Linux concerning hardware requirements

@subsection ver20 2.0 (Sept. 2010)

@li Initial release.

@author Michael Davidsaver <mdavidsaver@gmail.com>
@author Till Straumann <strauman@slac.stanford.edu>
@author Dirk Zimoch <dirk.zimoch@psi.ch>

*/

#endif // MAINPAGE_H
