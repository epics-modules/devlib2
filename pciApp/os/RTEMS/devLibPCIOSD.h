#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <rtems/irq.h>

/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_BASE_ADDRESS_0 + 4*(N) )

typedef uint32_t PCIUINT32;

#ifndef PCI_HEADER_TYPE_MASK
#  define PCI_HEADER_TYPE_MASK 0x3
#endif

#ifndef PCI_HEADER_MULTI_FUNC
#  define PCI_HEADER_MULTI_FUNC 0x80
#endif

#endif /* OSDPCI_H_INC */
