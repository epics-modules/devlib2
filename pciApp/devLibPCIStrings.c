#include <stdlib.h>
#include <stdio.h>
#include <epicsAssert.h>
#define epicsExportSharedSymbols
#include "devLibPCI.h"

const char* devPCIDeviceClassToString(int classId)
{
    struct {int classId; const char* name;} classes [] =
    {
      { 0x0000, "legacy device" },
      { 0x0001, "legacy VGA device" },

      { 0x0100, "SCSI controller" },
      { 0x0101, "IDE controller" },
      { 0x0102, "Floppy controller" },
      { 0x0103, "IPI controller" },
      { 0x0104, "RAID controller" },
      { 0x0105, "ATA controller" },
      { 0x0106, "SATA controller" },
      { 0x0107, "srial attached SCSI controller" },
      { 0x0180, "generic mass storage controller" },
      { 0x01ff, "unknown mass storage controller" },

      { 0x0200, "Ethernet controller" },
      { 0x0201, "Token Ring controller" },
      { 0x0202, "FDDI controller" },
      { 0x0203, "ATM controller" },
      { 0x0204, "ISDN controller" },
      { 0x0205, "WorldFip controller" },
      { 0x0206, "PCIMG controller" },
      { 0x0207, "Infiniband controller" },
      { 0x0280, "generic network controller" },
      { 0x02ff, "unknown network controller" },
      
      { 0x0300, "VGA controller" },
      { 0x0301, "XGA controller" },
      { 0x0302, "3D display controller" },
      { 0x0380, "generic display controller" },
      { 0x03ff, "unknown display controller" },

      { 0x0400, "video controller" },
      { 0x0401, "audio controller" },
      { 0x0402, "telephony controller" },
      { 0x0403, "audio controller" },
      { 0x0480, "generic multimedia controller" },
      { 0x04ff, "unknown multimedia controller" },

      { 0x0500, "RAM controller" },
      { 0x0501, "Flash controller" },
      { 0x0580, "generic memory controller" },
      { 0x05ff, "unknown memory controller" },

      { 0x0600, "PCI host bridge" },
      { 0x0601, "ISA bridge" },
      { 0x0602, "EISA bridge" },
      { 0x0603, "Micro Channel bridge" },
      { 0x0604, "PCI-to-PCI bridge" },
      { 0x0605, "PCMCIA bridge" },
      { 0x0606, "NuBus bridge" },
      { 0x0607, "Cardbus bridge" },
      { 0x0608, "RACEway bridge" },
      { 0x0609, "semi-transparent PCI-to-PCI bridge" },
      { 0x060a, "Infiniband PCI Host bridge" },
      { 0x0680, "generic bus bridge" },
      { 0x06ff, "unknown bus bridge" },

      { 0x0700, "serial port controller" },
      { 0x0701, "parallel port controller" },
      { 0x0702, "multi port serial controller" },
      { 0x0703, "modem" },
      { 0x0704, "GPIB controller" },
      { 0x0705, "SmartCard controller" },
      { 0x0780, "generic communication controller" },
      { 0x07ff, "unknown communication controller" },

      { 0x0800, "PIC controller" },
      { 0x0801, "DMA controller" },
      { 0x0802, "timer" },
      { 0x0803, "real time clock" },
      { 0x0804, "PCI hot-plug controller" },
      { 0x0805, "SD host controller" },
      { 0x0805, "IOMMU controller" },
      { 0x0880, "generic system peripheral" },
      { 0x08ff, "unknown system peripheral" },

      { 0x0900, "keyboard" },
      { 0x0901, "digitizer pen" },
      { 0x0902, "scanner" },
      { 0x0903, "gameport" },
      { 0x0980, "generic input device" },
      { 0x09ff, "unknown input device" },

      { 0x0a00, "docking station" },
      { 0x0a80, "generic docking station" },
      { 0x0aff, "unknown docking station" },

      { 0x0b00, "386 processor" },
      { 0x0b01, "486 processor" },
      { 0x0b02, "Pentium processor" },
      { 0x0b10, "Alpha processor" },
      { 0x0b20, "PowerPC processor" },
      { 0x0b30, "MIPS processor" },
      { 0x0b40, "co-processor" },
      { 0x0b80, "generic processor" },
      { 0x0bff, "unknown processor" },

      { 0x0c00, "FireWire controller" },
      { 0x0c01, "ACCESS controller" },
      { 0x0c02, "SSA controller" },
      { 0x0c03, "USB controller" },
      { 0x0c04, "Fibre Channel controller" },
      { 0x0c05, "SMBus controller" },
      { 0x0c06, "InfiniBand controller" },
      { 0x0c07, "IPMI SMIC controller" },
      { 0x0c08, "SERCOS controller" },
      { 0x0c09, "CANBUS controller" },
      { 0x0c80, "generic serial bus controller" },
      { 0x0cff, "unknown serial bus controller" },

      { 0x0d00, "IRDA controller" },
      { 0x0d01, "IR controller" },
      { 0x0d10, "RF controller" },
      { 0x0d11, "Bluetooth controller" },
      { 0x0d12, "Broadband controller" },
      { 0x0d20, "802.1a controller" },
      { 0x0d21, "802.1b controller" },
      { 0x0d80, "generic wireless controller" },
      { 0x0dff, "unknown wireless controller" },

      { 0x0e00, "I2O controller" },
      { 0x0e80, "generic intelligent controller" },
      { 0x0eff, "unknown intelligent controller" },

      { 0x0f01, "satellite TV controller" },
      { 0x0f02, "satellite audio controller" },
      { 0x0f03, "satellite video controller" },
      { 0x0f04, "satellite data communications controller" },
      { 0x0f80, "generic satellite communications controller" },
      { 0x0fff, "unknown satellite communications controller" },

      { 0x1000, "network and computing encryption device" },
      { 0x1001, "entertainment encryption device" },
      { 0x1080, "generic encryption device" },
      { 0x10ff, "unknown encryption device" },

      { 0x1100, "DPIO controller" },
      { 0x1101, "performance counter" },
      { 0x1110, "communication synchronizer" },
      { 0x1120, "signal processing management" },
      { 0x1180, "generic signal processing controller" },
      { 0x11ff, "unknown signal processing controller" },

      { 0x1200, "processing accelerator" },
      { 0x1280, "generic processing accelerator" },
      { 0x12ff, "unknown processing accelerator" },

      { 0x1300, "non-essential instrumentation" },
      { 0x1380, "generic non-essential instrumentation" },
      { 0x13ff, "unknown non-essential instrumentation" },

      { 0xffff, "unknown device class" }
    };
    
    unsigned int n = 0;
    const unsigned int nument = sizeof(classes)/sizeof(classes[0])-1;
    unsigned int m = nument;
    unsigned int i;
    
    classId >>= 8;

    /* binary search */
    while (m>n)
    {
        i = (n+m)>>1;
        assert(i>=0 && i<nument);
        if (classes[i].classId == classId) return classes[i].name;
        if (classes[i].classId < classId) n=i+1;
        if (classes[i].classId > classId) m=i;
    }
    if(i<nument-1) {
        /* unknown device class linear search */
        classId |= 0xff;
        assert(i>=0 && i<nument-1);
        while (classes[++i].classId < classId);
        return classes[i].name;
    } else {
        /* unknown device class */
        return classes[nument].name;
    }
}
