Record recipies for PCI "explorer" IOC
======================================

INP/OUT field values
--------------------

All records with DTYP="Explore ..." should begin the INP or OUT string
in one of the following formats.

* "@BB:DD.FF"
* "@slot=#"

Where "BB:DD.FF" is a PCI geographic address (bus, device, function),
and "slot=SN" gives a PCI slot number (when OS support is present).

A space seperated list of the following may follow.

* "bar=#" (default: 0)
* "offset=#" in bytes (default: 0)
* "mask=#" bit mask (default: 0 aka. no mask)
* "shift=#" in bits (default: 0)
* "step=#" in bytes (default: 0)

```
  field(INP , "@8:0.0 bar=1 offset=0x14")
... or ...
  field(OUT , "@8:0.0 bar=1 offset=0x14")
```

Selects device "8:0.0" BAR 1 starting with byte offset 0x14.

Numbers prefixed with '0x' are hexidecimal.  Numbers w/o a prefix are decimal.

Optional for all input/read records
-----------------------------------

```
  field(PINI, "YES")
```

Read once on IOC startup

```
  field(SCAN, "1 second")
```

Read periodically, ever 1 second.  Other options are ".1 second", ".2 second",
".5 second", "2 second", "5 second", and "10 second".


Read a scalar value
-------------------

Reading a 32-bit register as an scalar integer in little endian byte order.
Reads periodically at 1Hz.

```
record(longin, "pcitest0") {
  field(DTYP, "Explore Read32 LSB")
  field(INP , "@8:0.0 bar=0 offset=0")
  field(SCAN, "1 second")
}
```

Read an array of values
-----------------------

```
record(waveform, "pcitest0_10") {
  field(DTYP, "Explore Read32 LSB")
  field(INP , "@8:0.0 bar=0 offset=8 step=4")
  field(FTVL, "ULONG")
  field(NELM, "16")
  field(SCAN, "1 second")
}
```

Makes 16 sequential reads of 4 bytes each.
The first read is of address 8.

The number of elements to read is determined by the NELM field.

Write a scalar value
--------------------

```
record(longout, "pcitestout") {
  field(DTYP, "Explore Write32 LSB")
  field(OUT , "@8:0.0 bar=0 offset=0xc")
}
```

Make a single 4 byte write to address 0xc

Bit-field access
----------------

A read-modify-write operation to address 0xc.

```
record(longout, "pcitestout") {
  field(DTYP, "Explore Write32 LSB")
  field(OUT , "@8:0.0 0 offset=0xc mask=0xff00 shift=8")
}
```

Equivlant pseudo-code.

```
u32 newval = read()&~mask
newval |= (VAL<<shift)&mask
write(newval)
```

A masked read

```
record(longin, "pcitestin") {
  field(DTYP, "Explore Read32 LSB")
  field(OUT , "@8:0.0 0 offset=0xc mask=0xff00 shift=8")
}
```

Equivlant pseudo-code.

```
VAL = (read()&mask)>>shift
```
