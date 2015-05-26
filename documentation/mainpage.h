#ifndef MAINPAGE_H
#define MAINPAGE_H

/**

@mainpage devLib2 MMIO Bus Access Library

@section whatisit What is it?

devLib2 is an extension to the EPICS OS independent VME bus access
library (devLib v1) found in the 3.14.x series.
The v2 library is an overlay and extension to the v1 library and
not a replacement.
It is planned that the v2 library will be merged with the v1 library
for the 3.16.x series.
After that point devlib2 will continue to exist as a location for backports
and bug fixes for the 3.14.x and 3.15.x series.

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
RTEMS and vxWorks.  An implementation for userspace Linux is also @ref pcilinux "under development."
The PCI interface provides functions for: searching the bus, mapping devices
into process memory, and (dis)connecting interrupts.

Runtime selection of implementations is also supported.
This allows code using this interface to compile and link for all target,
even those which lack an implementation.

@li @ref pci "API Documentation"

@li @subpage pciusage

@subsection vmesec VME CSR

The VME64x library provides several functions for accessing the CSR/CR
address space defined by VME64 and extended by VME64x.
This includes: probing by slot number and matching by identifier registers,
definitions of standard registers, and functions to access them.

@li @ref vmecsr "API Docmentation"

@li @subpage vmeusage

@li @ref vmecsrregs "CSR/CR Register Definitions"

@subsection mmiosec MMIO

The MMIO library provides an OS and CPU architecture independent way to
safely access memory mapped I/O devices.  Makes guarantees of
the width and order of accesses.

@li @ref mmio "API Docmentation"

@section changelog Changelog

@subsection ver27 2.7 (UNRELEASED)

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

@author Michael Davidsaver <mdavidsaver@bnl.gov>
@author Till Straumann <strauman@slac.stanford.edu>
@author Dirk Zimoch <dirk.zimoch@psi.ch>

*/

/**

@page pciusage PCI Usage

When working with a PCI device the first step is finding the identifiers
used by the device.
These will likely be included in the card vendor's documentation,
but can be verified by observing the OS boot messages or with the devPCIShow()
function.

Including this identifying information in your code provides an important
safe guard against user mis-configuration.  This provides an easy way
to prevent your code from trying to access the wrong type of device.

PCI devices are uniquely identified by the triplet bus:device.function.
While it is possible
to automatically detect cards it is preferable to have a user defined
mapping between physical id (b:d.f) and logical id (Asyn port, device ID, etc.).
This allows for consistent naming even if cards are added or removed
from the system.

For EPICS drivers initialization will usually be done with an IOC shell
function.  For example:

@code
myPCICardSetup("dac", 2, 1, 0 )
@endcode

Would set card "dac" to be function 0 of device 1 on bus 2.
Since all PCI devices will be automatically configured by the system
this is the only piece of information required to access the card.

Below is an example implementation of myPCICardSetup().

@code
static const epicsPCIID mydevids[] = {...}

int
myPCICardSetup(const char* port,
               int b,
               int d,
               int f)
{
    volatile void* bar;
    epicsPCIDevice *dev;
    devpriv *priv;

    if( portExists(port) ) return 1;

    if (devPCIFindBDF(mydevids, b,d,f, &dev, 0))
        return 2;
@endcode

The first step is to probe the PCI location to ensure that the card is present.
If the location is empty or populated with an unsupported card then we will abort here.

@code
    if (devPCIToLocalAddr(dev, 2, &bar, 0))
        return 3;
@endcode

Next we get a pointer for bar #2.

@code
    priv=calloc(1, sizeof(mydevpriv));
    if (!priv) return 4;

    if (devPCIConnectInterrupt(dev, &myisr, priv))
        return 5;
@endcode

Then connect the interrupt service routine.

@code
    portCreate(port, priv);
    return 0;
}
@endcode

And that is it.

@section pciiotypes Note on PCI I/O Types

The PCI bus has the notion of two different I/O address types:
Memory Mapped, and Port.
This distinction comes from the x86 processor for which these are
two separate address spaces which must be accessed using different
instructions (load/store vs. out/in).

Each BAR must either be accessed using Memory Mapped I/O (MMIO),
or Port I/O (PIO) operations. This can be determined by inspecting
the 'ioport' member of the ::PCIBar structure.

If the value is false (0) then @ref mmio "MMIO operations" should be used.

If however, the value is true (1) then the pointer returned by devPCIToLocalAddr()
needs to be treated as a PIO address.  It should be cast to the appropriate type
and access using OS defined PIO operations (ie inb/outb).

*/

/**

@page vmeusage VME64 CSR Usage

When working with a CSR/CR device the first step is to find
the identifiers for the board.
This should be included in vendor documentation,
and can be verified by probing the appropriate VME slot at runtime.
The library functions vmecsrdump(lvl) and vmecsrprint(slot,lvl)
can be used for this.  See the IOC shell section for detail on
these functions.

Including this identifying information in your code provides an important
safe guard against user mis-configuration.  This provides an easy way
to prevent your code from trying to access the wrong type of device.

To initialize you must know its slot number.  While it is possible
to automatically detect cards it is preferable to have a user defined
mapping between physical id (slot#) and logical id (Asyn port, device ID, etc.).
This allows for consistent naming even if cards are added or removed
from the system.

For EPICS drivers initialization will usually be done with an IOC shell
function.  For example:

@code
myVMECardSetup("dac", 5, 0x210000, 4, 0x60)
@endcode

Would set card "dac" to be the card in slot 5.
This imaginary card is given an A24 base address of 0x210000,
and set to generate interrupt vector 0x60 at level 4.

Below is an example implementation of myVMECardSetup().

@code
static const struct VMECSRID mydevids[] = {...}

int
myVMECardSetup(const char* port,
               int slot,
               unsigned long base,
               int level,
               int vector)
{
    volatile void* csr, a24;
    devpriv *priv;
    if( portExists(port) ) return 1;

    csr=devCSRTestSlot(mydevids, slot, NULL);
    if (!csr) return 2;
@endcode

The first step is to probe the VME slot and get the CSR
base address.  If the slot were empty or populated with
an unsupported card then we would abort here.

@code
#if card is VME64
    CSRWrite24(csr + MYCSR_BAR_BASE, base);

#elif card is VME64x
    CSRSetBase(csr, 0, base, VME_AM_STD_SUP_DATA);

#endif
@endcode

Assuming the card supports full jumperless configuration then
then base address will be programmed using the CSR space.
While newer VME64x cards have standard base address registers
for this, older VME64 cards do not.
For these card the vendor should document the correct way to
set the base address.

@code
    if (devRegisterAddress("mydrv", atVMEA24, base, 0xff, &a24))
        return 3;
@endcode

Once the A24 base address is set we map it into the process'
address space with the standard devLib mapping call.

@code
    priv=calloc(1, sizeof(mydevpriv));
    if (!priv) return 4;

    if (devConnectInterruptVME(vector&0xff, &myisr, priv))
        return 5;

    if (devEnableInterruptLevelVME(level&0x7)) return 6;
@endcode

Always attach the interrupt handler before enabling interrupts.

@code
    CSRWrite8(csr + MYCSR_IRQ_LEVEL, level&0x7);
    CSRWrite8(csr + MYCSR_IRQ_VECTOR, vector&0xff);
@endcode

Neither VME64 or VME64x specifies standard registers for
interrupt level or vector code.  Vendor documention must
specify how to set this.

.Note
*************************************************
This may not be done in the CSR space, but rather
in another address space.
*************************************************

@code
    portCreate(port, priv);
    return 0;
}
@endcode
*/

/**
@page pcilinux PCI Access in Linux

The PCI bus implementation for Linux uses the Userspace IO kernel API to access the bus.
To support this an additional small kernel module must be written for each device
being supported.
The main purpose of the kernel module is to provide an interrupt handler in kernel context
to silence the interrupt.
The UIO framework then notifies the userspace application that an interrupt has occurred.
Once serviced the interrupt must be reenabled by the application.

A UIO kernel module will expose several memory regions.
These can be MMIO or main memory.
The devLib2 PCI driver treats each as a PCI BAR.

@note Support for Linux is considered beta quality.  All testing result (positive or negative) are welcomed.

@section shouldi When to Use

It is important to consider writing a full kernel module.
The advantage of the UIO interface is simpler development with access to userspace tools and safeguards.
Also compatibility with RTEMS and vxWorks.
The disadvantage is increased latency, being forced to be compatible with RTEMS and vxWorks,
and being restricted in use to EPICS code.

It is likely that devices needing low latency, high throughput, or access to features like DMA
would be better served by writing a full kernel module.

@section hwrequire Hardware Requirements

When servicing interrupts from a userspace process it is not possible to globally
disable interrupts.  Thus it is not possible to rely on epicsInterruptLock()
when accessing resources which must be shared between kernel and user-space.

This makes it difficult to perform read-modify-write operations
on shared registers.  This can be an intractable problem
for a device which implements its interrupt enable register as a bit mask.

When evaluating hardware for the suitability look for a single register
which can disable/enable interrupts without resetting interrupt active
flags.

This should not be an issue for devices having a single interrupt condition.
Devices using PCI to local bus bridges like the PLX PCI9030 should be usable.
Also, devices supporting version 2.3 or greater of PCI standard can make use
of the standard master interrupt enable/status bits in the control and status
registers.

@section linuxrefs References

More information on writing UIO kernel modules can be found:

\see http://www.kernel.org/doc/htmldocs/uio-howto.html
\see http://lwn.net/Articles/232575/

The kernel source tree contains several example drivers.

\see http://lxr.linux.no/#linux+v2.6.35/drivers/uio/uio_cif.c

*/

#endif // MAINPAGE_H
