Record recipies for PCI "explorer" IOC
======================================

INP/OUT field values
--------------------

All records with DTYP="Explore PCI ..." should set the INP or OUT
with the format "@BB:DD.FF BAR# OFFSET".
Where "BB:DD.FF" is a PCI geographic address (bus, device, function)
along with a memory region number (0-6) and an offset into that region
given in bytes.

```
  field(INP , "@8:0.0 1 14")
... or ...
  field(OUT , "@8:0.0 1 14")
```

Selects device "8:0.0" BAR 1 starting with byte offset 0x14.

All number in this string are given in hexadecimal.

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

Reading a 32-bit register as an scalar integer in big endian byte order.
Reads periodically at 1Hz.

```
record(longin, "pcitest0") {
  field(DTYP, "Explore PCI Read32")
  field(INP , "@8:0.0 0 0")
  field(SCAN, "1 second")
}
```

Read an array of values
-----------------------

```
record(waveform, "pcitest0_10") {
  field(DTYP, "Explore PCI Read32")
  field(INP , "@8:0.0 0 8")
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
  field(DTYP, "Explore PCI Write32")
  field(OUT , "@8:0.0 0 c")
}
```

Make a single 4 byte write to address 0xc
