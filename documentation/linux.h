
/**
@page pcilinux PCI Access in Linux

The PCI bus implementation for Linux uses the Userspace IO kernel API to access the bus.
Devices needing only memory mapped I/O access do not require a kernel driver.
To support PCI style interrupts a minimal kernel module using the Linux UIO framework is required.
Devices which implement generic interrupt enable/disable via the PCI control register
may use the generic 'uio_pci_generic' kernel module.
Otherwise this must either be written for each device to be supported.

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

When evaluating hardware for the suitability look for an
interrupt mask register and status register.

This should not be an issue for devices having a single interrupt condition.
Devices using PCI to local bus bridges like the PLX PCI9030 should be usable.
Also, devices supporting version 2.3 or greater of PCI standard can make use
of the standard master interrupt enable/status bits in the control and status
registers.

@section udev UDEV rules

To allow IOCs to run with minimal privlages it is advisable to change the permissions
of the special files use by devlib2.
This can be done either manually, or by configuring the UDEV daemon.

When attempting to make use of a PCI devices, devlib2 will attempt to access several files.

@li /sys/bus/pci/devices/000:BB:DD.F/resource#

The "resource*" files exist one for each BAR.
devPCIToLocalAddr() will first attempt to open the corresponding file.

@li /sys/bus/pci/devices/000:BB:DD.F/uio:uio#  (Linux <= 2.6.28)
@li /sys/bus/pci/devices/000:BB:DD.F/uio/uio#  (Linux >= 2.6.32)

These are possible locations of symlinks to "/dev/uio#".
devPCIToLocalAddr() will next attempt to open each in turn,
then map the a region corresponding to the BAR number,
or the number of mappings when the DEVLIB_MAP_UIOCOMPACT flag is given.

In this example, the permissions of /resource0 are changed.

@code
SUBSYSTEM=="pci", ATTR{vendor}=="0x1234", ATTR{device}=="0x1234", RUN+="/bin/chmod a+rw %S%p/resource0"
@endcode

And for a UIO device

@code
SUBSYSTEM=="pci", ATTR{vendor}=="0x1234", ATTR{device}=="0x1234", MODE="0666"
@endcode

To discover possible attributes for matching use "udevadm info" and look for the first
SUBSYSTEM=="pci" paragraph.

@code
udevadm info -a -p $(udevadm info -q path -n /dev/uio0) --attribute-walk
@endcode

If not device file exists, then the PCI geographic address can be used

@code
udevadm info -a -p /bus/pci/devices/0000:0b:00.0 --attribute-walk
@endcode

The effects of a new rules file can be tested with "udevadm -a add test".

@code
udevadm test -a add /bus/pci/devices/0000:0b:00.0
udevadm test -a add $(udevadm info -q path -n /dev/uio0)
@endcode

@section linuxrefs References

More information on writing UIO kernel modules can be found:

\see http://www.kernel.org/doc/htmldocs/uio-howto.html
\see http://lwn.net/Articles/232575/

The kernel source tree contains several example drivers.

\see http://lxr.linux.no/#linux+v2.6.35/drivers/uio/uio_cif.c

*/
