
/** @page exploreapp PCI driver/hardware development tool

@section exploreintro Purpose

The exploreApp support is intended as a tool to explore new PCI devices.
This may be useful when preparing to write a dedicated driver and/or
during development of PCI/PCIe device firmware.
exploreApp allows a PCI register to be read and/or written an EPICS record.
For example:

@code
record(longout, "pcitestout") {
  field(DTYP, "Explore Write32 LSB")
  field(OUT , "@8:0.0 bar=0 offset=0xc")
}
@endcode

The record @b pcitestout will be connected to the PCI device 8:0.0 (bus 8, device 0, function 0) with the first BAR.
A single write of 4 bytes is made when the record is processed.

The following can be added to some @b xyzApp/src/Makefile to include exploreApp.

@code
PROD_IOC += myioc
DBD += myioc.dbd
myioc_DBD += exploreSupport.dbd
myioc_LIBS += explorepci epicspci
@endcode

@section exploreopts Options

@b INP/OUT link strings may contain the following components

@li "bar=#" (default: 0)
@li "offset=#" in bytes (default: 0)
@li "mask=#" bit mask (default: 0 aka. no mask)
@li "shift=#" in bits (default: 0)
@li "step=#" in bytes (default: read size.  eg Read32 defaults to step=4)
@li "initread=1|0" bool (default: 1 for .OUT recordtypes, 0 otherwise)


For record types: @b longout, @b bo, @b mbbo, @b mbboDirect, @b ao
allowed @b DTYP are:

@li Explore Write8
@li Explore Write16 NAT
@li Explore Write16 LSB
@li Explore Write16 MSB
@li Explore Write32 NAT
@li Explore Write32 LSB
@li Explore Write32 MSB

For record types: @b longin, @b longout, @b bi, @b bo, @b mbbi, @b mbbo, @b mbbiDirect, @b mbboDirect, @b ai, @b ao

@li Explore Read8
@li Explore Read16 NAT
@li Explore Read16 LSB
@li Explore Read16 MSB
@li Explore Read32 NAT
@li Explore Read32 LSB
@li Explore Read32 MSB

The @b ao record type also accepts

@li Explore WriteF32 LSB
@li Explore WriteF32 MSB

The @b ai record type also accepts

@li Explore ReadF32 LSB
@li Explore ReadF32 MSB

The @b waveform record type accepts both integer Read and Write @b DTYP.
The @b step= link option may be applied to change how the address counter is incremented.
The default step size is the read size (eg. 4 for Read32).
A step size of 0 will read the base address @b NELM times.

@section exploreirq PCI Interrupt

Limited support of PCI interrupts is available on Linux only.
A @b longin record with DTYP="Explore IRQ Count" and SCAN="I/O Intr"
will be processed each time an interrupt occurs.

This requires that a UIO kernel module be installed.

@section explorefrib FRIB Specific

The DTYP="Explore FRIB Flash" support implements a FRIB specific protocol
for accessing a SPI flash chip over PCI.
The @b frib-flash.db file demonstrates use.

*/

/** @page iocsh IOC shell functions

Several IOC shell functions are provided to access PCI and VME devices

@section iocshpci IOCsh functions for PCI devices

- devPCIShow() List PCI devices present
- devLibPCIUse() Select PCI system access implementation
- pcidiagset() Select device for read/write functions
- pciwrite()
- pciread()
- pciconfread()

To use, begin by calling pcidiagset() to select the device and BAR that subsequent
read/write calls will operate on.

@section iocshvme IOCsh functions for VME devices

- vmeread()
- vmewrite()
- vmeirqattach()
- vmeirq()

*/
