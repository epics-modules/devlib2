/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2006 The University of Chicago,
*     as Operator of Argonne National Laboratory.
* Copyright (c) 2006 The Regents of the University of California,
*     as Operator of Los Alamos National Laboratory.
* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
*     University, as Operator of the Stanford Linear Accelerator Center.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef DEVLIBCSR_H
#define DEVLIBCSR_H 1

/**
 * @defgroup vmecsr VME CSR
 *
 * Extensions to EPICS devLib to deal with the CSR address space defined
 * by the VME64 standard and extended by the VME64x standard.
 * @{
 */

#include <epicsVersion.h>

#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION<10
#  include "devLibVME.h"
#else
#  include "devLib.h"
#endif

#include <epicsTypes.h>
#include "epicsMMIO.h"
#include "vmedefs.h"

#ifdef __cplusplus
extern "C" {

#  ifndef INLINE
#    define INLINE static inline
#  endif
#endif

#define DEVLIBVME_MAJOR 1 /**< @brief API major version */
#define DEVLIBVME_MINOR 0 /**< @brief API minor version */

/** @brief ID info for a VME64(x) device
 * This structure is used to hold identifying information for a VME64
 * compatible device.  When used for searching each field can hold
 * a specific value of the 'VMECSRANY' wildcard.
 */
struct VMECSRID {
	epicsUInt32 vendor,board,revision;
};

/** @brief Must be the last entry in a device list */
#define VMECSR_END {0,0,0}

/** @brief Match any value.  May be used in any field of ::VMECSRID */
#define VMECSRANY 0xFfffFfff

/** @brief The highest slot number. */
#define VMECSRSLOTMAX ((1<<5)-1)

/** @brief Get the CSR base address for a slot
  *
  * Test a single slot for the presense of VME64 complient module.
  * Succeeds if a card is present and provides the standard
  * registers.
  *
  * @warning Cards which do not provide the CR_ASCII_C and CR_ASCII_R registers
  *          with the correct values ('C' and 'R') will be ignored.  This rejects
  *          cards which use the CSR address space for some other purpose and
  *          don't provide the standard registers.
  *
  @param slot The VME slot number (0-31)
  @retval NULL  On all failures
  @retval !NULL A pointer to slot's CSR base address
  */
epicsShareFunc
volatile unsigned char* devCSRProbeSlot(int slot);

/** @brief Probe a VME slot then check its ID
  *
  * Calls devCSRProbeSlot().  If a card is found the PCI ID
  * fields are compared against the given VMECSRID list.
  * The base address is returned if the card matches and NULL
  * otherwise.
  *
  * If info is non-NULL then the structure it points to will be filled
  * with the ID information of the matching card.
  *
  * An identifier list should be defined like:
  @code
    static const struct VMECSRID vmedevices[] = {
        {0x123456, 0x87654321, 0x15}
       ,{0x123456, 0x87654321, 0x16}
       ,{0x123456, 0x87655678, VMECSRANY}
       ,{0x214365, VMECSRANY, VMECSRANY}
       ,VMECSR_END
    };
  @endcode
  *
  @param devs A list of PCI device IDs (wildcards optional).
  @param slot VME slot number (0-31)
  @param info If not NULL the exact ID of the matching device is copied here.
  @retval NULL It no card is present, or the card does not match.
  @retval !NULL If a card matching an entry in the device list is found.
  */
epicsShareFunc
volatile unsigned char* devCSRTestSlot(
	const struct VMECSRID* devs,
	int slot,
	struct VMECSRID* info
);

/** @brief Derives the CSR space base address for a slot.
 *
 * The top 5 bits of the 24 bit CSR address are the slot number.
 * 
 * This macro gives the VME CSR base address for a slot.
 * Give this address to devBusToLocalAddr() with type atVMECSR
 */
#define CSRSlotBase(slot) ( (slot)<<19 )

/** @brief Computes values for the VME64x address decode registers (ADER).
 *
 * The ADER register encodes the address modifier and base address
 * on one address range.
 */
#define CSRADER(addr,mod) ( ((addr)&0xFfffFf00) | ( ((mod)&0x3f)<<2 ) )

/* Read/Write CR for standard sizes
 */

#define CSRRead8(addr) ioread8(addr)

#define CSRRead16(addr) ( CSRRead8(addr)<<8 | CSRRead8(addr+4) )

#define CSRRead24(addr) ( CSRRead16(addr)<<8 | CSRRead8(addr+8) )

#define CSRRead32(addr) ( CSRRead24(addr)<<8 | CSRRead8(addr+12) )

#define CSRWrite8(addr,val) iowrite8(addr, val)

#define CSRWrite16(addr,val) \
do{ CSRWrite8(addr,(val&0xff00)>>8); CSRWrite8(addr+4,val&0xff); }while(0)

#define CSRWrite24(addr,val) \
do{ CSRWrite16(addr,(val&0xffff00)>>8); CSRWrite8(addr+8,val&0xff); }while(0)

#define CSRWrite32(addr,val) \
do{ CSRWrite24(addr,(val&0xffffff00)>>8); CSRWrite8(addr+12,val&0xff); }while(0)

/*
 * Utility functions
 */

/** @brief Decode contents of CSR/CR and print to screen.
 *
 * @li v=0 - basic identification info (vendor/board id)
 * @li v=1 - config/capability info
 * @li v=2 - hex dump of start of CR
 *
 @param N VME slot number (0-31)
 @param verb Level of detail (0-2)
 */
epicsShareExtern void vmecsrprint(int N,int verb);

/** @brief Decode contents of CSR/CR for all cards and print to screen.
 *
 * Calls vmecsrprint() on all slots (0-21)
 *
 @param verb Level of detail (0-2)
 */
epicsShareExtern void vmecsrdump(int verb);

/** @} */
/** @defgroup vmecsrregs VME CSR Register Definitions
 *
 * Common defininitions for registers found
 * in the Configuration Rom (CR) on VME64 and VME64x cards.
 *
 * These registers are addressed with the CSR
 * address space.
 *
 * The CR is a little strange in that all values are
 * single bytes (D8), but still have 4 byte spacing.
 * For example the Organizationaly Unique Identifier (OUI)
 * is 3 bytes long.  The first byte is offset 0x27, the
 * second is 0x2B, and the third is 0x2F.
 *
 * The following definitions were originally taken from the
 * mrfEventSystem IOC written by:
 *   Jukka Pietarinen (Micro-Research Finland, Oy)
 *   Till Straumann (SLAC)
 *   Eric Bjorklund (LANSCE)
 *
 * Corrected against 'The VMEBus Handbook' (Ch 5.6)
 * ISBN 1-885731-08-6
 *@{
 */

/**************************************************************************************************/
/*  CR/CSR Configuration ROM (CR) Register Definitions                                            */
/**************************************************************************************************/

/* VME64 required CR registers */

#define  CR_ROM_CHECKSUM           0x0003 /**< @brief 8-bit checksum of Configuration ROM space   */
#define  CR_ROM_LENGTH             0x0007 /**< @brief Number of bytes in Configuration ROM to checksum     */
#define  CR_DATA_ACCESS_WIDTH      0x0013 /**< @brief Configuration ROM area (CR) data access method       */
#define  CSR_DATA_ACCESS_WIDTH     0x0017 /**< @brief Control/Status Reg area (CSR) data access method     */
#define  CR_SPACE_ID               0x001B /**< @brief CR/CSR space ID (VME64, VME64X, etc).                */

#define  CR_ASCII_C                0x001F /**< @brief ASCII "C" (identifies this as CR space)              */
#define  CR_ASCII_R                0x0023 /**< @brief ASCII "R" (identifies this as CR space)              */

#define  CR_IEEE_OUI               0x0027 /**< @brief IEEE Organizationally Unique Identifier (OUI)        */
#define  CR_IEEE_OUI_BYTES                     3   /**< @brief Number of bytes in manufacturer's OUI       */
#define  CR_BOARD_ID               0x0033 /**< @brief Manufacturer's board ID                              */
#define  CR_BOARD_ID_BYTES                     4   /**< @brief Number of bytes in manufacturer's OUI       */
#define  CR_REVISION_ID            0x0043 /**< @brief Manufacturer's board revision ID                     */
#define  CR_REVISION_ID_BYTES                  4   /**< @brief Number of bytes in board revision ID        */
#define  CR_ASCII_STRING           0x0053 /**< @brief Offset to ASCII string (manufacturer-specific)       */
#define  CR_PROGRAM_ID             0x007F /**< @brief Program ID code for CR space                         */

/* VME64x required CR registers */

#define  CR_BEG_UCR                0x0083 /**< @brief Offset to start of manufacturer-defined CR space     */
#define  CR_END_UCR                0x008F /**< @brief Offset to end of manufacturer-defined CR space       */
#define  CR_BEG_UCSR_BYTES                     3   /**< @brief Number of bytes in User CSR starting offset */

#define  CR_BEG_CRAM               0x009B /**< @brief Offset to start of Configuration RAM (CRAM) space    */
#define  CR_END_CRAM               0x00A7 /**< @brief Offset to end of Configuration RAM (CRAM) space      */

#define  CR_BEG_UCSR               0x00B3 /**< @brief Offset to start of manufacturer-defined CSR space    */
#define  CR_END_UCSR               0x00BF /**< @brief Offset to end of manufacturer-defined CSR space      */

#define  CR_BEG_SN                 0x00CB /**< @brief Offset to beginning of board serial number           */
#define  CR_END_SN                 0x00DF /**< @brief Offset to end of board serial number                 */

#define  CR_SLAVE_CHAR             0x00E3 /**< @brief Board's slave-mode characteristics                   */
#define  CR_UD_SLAVE_CHAR          0x00E7 /**< @brief Manufacturer-defined slave-mode characteristics      */

#define  CR_MASTER_CHAR            0x00EB /**< @brief Board's master-mode characteristics                  */
#define  CR_UD_MASTER_CHAR         0x00EF /**< @brief Manufacturer-defined master-mode characteristics     */

#define  CR_IRQ_HANDLER_CAP        0x00F3 /**< @brief Interrupt levels board can respond to (handle)       */
#define  CR_IRQ_CAP                0x00F7 /**< @brief Interrupt levels board can assert                    */

#define  CR_CRAM_WIDTH             0x00FF /**< @brief Configuration RAM (CRAM) data access method)         */

#define  CR_FN_DAWPR(N)  ( 0x0103 + (N)*0x04 ) /* N = 0 -> 7 */
                                          /**< @brief Start of Data Access Width Parameter (DAWPR) regs    */
#define  CR_DAWPR_BYTES                        1   /* Number of bytes in a DAWPR register         */

#define  CR_FN_AMCAP(N)  ( 0x0123 + (N)*0x20 ) /* N = 0 -> 7 */
                                          /**< @brief Start of Address Mode Capability (AMCAP) registers   */
#define  CR_AMCAP_BYTES                        8   /* Number of bytes in an AMCAP register        */

#define  CR_FN_XAMCAP(N) ( 0x0223 + (N)*0x80 ) /* N = 0 -> 7 */
                                          /**< @brief Start of Extended Address Mode Cap (XAMCAP) registers*/
#define  CR_XAMCAP_BYTES                      32   /* Number of bytes in an XAMCAP register       */

#define  CR_FN_ADEM(N)   ( 0x0623 + (N)*0x10 ) /* N = 0 -> 7 */
                                          /**< @brief Start of Address Decoder Mask (ADEM) registers       */
#define  CR_ADEM_BYTES                         4   /* Number of bytes in an ADEM register         */

#define  CR_MASTER_DAWPR           0x06AF /**< @brief Master Data Access Width Parameter                   */
#define  CR_MASTER_AMCAP           0x06B3 /**< @brief Master Address Mode Capabilities   (8 entries)       */
#define  CR_MASTER_XAMCAP          0x06D3 /**< @brief Master Extended Address Mode Capabilities (8 entries)*/

/*---------------------
 * Size (in total bytes) of CR space
 */
#define  CR_SIZE                          0x0750   /**< @brief Size of CR space (in total bytes)           */
#define  CR_BYTES                   (CR_SIZE>>2)   /**< @brief Number of bytes in CR space                 */

/**************************************************************************************************/
/*  CR/CSR Control and Status Register (CSR) Offsets                                              */
/**************************************************************************************************/

/* VME64 required CSR registers */

#define  CSR_BAR                0x7ffff /**< @brief Base Address Register (MSB of our CR/CSR address)      */
#define  CSR_BIT_SET            0x7fffb /**< @brief Bit Set Register (writing a 1 sets the control bit)    */
#define  CSR_BIT_CLEAR          0x7fff7 /**< @brief Bit Clear Register (writing a 1 clears the control bit)*/

/* VME64x required CSR registers */

#define  CSR_CRAM_OWNER         0x7fff3 /**< @brief Configuration RAM Owner Register (0 = not owned)       */
#define  CSR_UD_BIT_SET         0x7ffef /**< @brief User-Defined Bit Set Register (for user-defined fns)   */
#define  CSR_UD_BIT_CLEAR       0x7ffeb /**< @brief User-Defined Bit Clear Register (for user-defined fns) */
#define  CSR_FN_ADER(N) (0x7ff63 + (N)*0x10) /* N = 0 -> 7 */
                                        /**< @brief Function N Address Decoder Compare Register (1st byte) */
#define  CSR_ADER_BYTES                     4   /* Number of bytes in an ADER register            */

/*---------------------
 * Bit offset definitions for the Bit Set Status Register
 */
#define  CSR_BITSET_RESET_MODE           0x80   /**< @brief Module is in reset mode                        */
#define  CSR_BITSET_SYSFAIL_ENA          0x40   /**< @brief SYSFAIL driver is enabled                      */
#define  CSR_BITSET_MODULE_FAIL          0x20   /**< @brief Module has failed                              */
#define  CSR_BITSET_MODULE_ENA           0x10   /**< @brief Module is enabled                              */
#define  CSR_BITSET_BERR                 0x08   /**< @brief Module has asserted a Bus Error                */
#define  CSR_BITSET_CRAM_OWNED           0x04   /**< @brief CRAM is owned                                  */
/** @} */

/* Common things to set in CSR
 */

/** @brief Set base address for VME64x function N
 *
 @param base The CSR base address of a slot
 @param N A ADER number (0-7)
 @param addr Base address to set for given ADER
 @param amod VME address modifier to use for the given ADER
 *
 @ingroup vmecsr
 */
INLINE
void
CSRSetBase(volatile void* base, epicsUInt8 N, epicsUInt32 addr, epicsUInt8 amod)
{
  volatile char* ptr=(volatile char*)base;
  if (N>7) return;
  CSRWrite32((ptr) + CSR_FN_ADER(N), CSRADER(addr,amod) );
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBCSR_H */
