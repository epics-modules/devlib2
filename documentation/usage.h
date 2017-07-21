
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
