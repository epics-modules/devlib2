
devLib2 - Library for direct MMIO access to PCI and VME64x

Michael Davidsaver <mdavidsaver@bnl.gov>


devLib2 is an extension to the EPICS OS independent VME bus access
library (devLib v1) found in the 3.14.x series.
The v2 library is an overlay and extension to the v1 library and
not a replacement.
It is planned that the v2 library will be merged with the v1 library
for the 3.15.x series.
After that point devlib2 will continue to exist as a location for backports
and bug fixes for the 3.14.x series.

Requires:

EPICS Base >= 3.14.8.2

http://www.aps.anl.gov/epics/

For details see documentation/mainpage.h
This is formatted for the Doxygen documentation generator (http://www.doxygen.org/).

$ cd documentation
$ doxygen Doxyfile
...

