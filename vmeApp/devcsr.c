/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* devLib2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdio.h>

#include <errlog.h>
#include "devLib.h"
#define epicsExportSharedSymbols
#include "devcsr.h"

epicsShareFunc
volatile unsigned char* devCSRProbeSlot(int slot)
{
  volatile unsigned char* addr;
  char cr[3];

  if(slot<0 || slot>VMECSRSLOTMAX){
    errlogPrintf("VME slot number out of range\n");
    return NULL;
  }

  if( devBusToLocalAddr(
         atVMECSR,
         CSRSlotBase(slot),
         (volatile void**)(void*)&addr) )
  {

    errlogPrintf("Failed to map slot %d to CR/CSR address 0x%08lx\n",slot,
           (unsigned long)CSRSlotBase(slot));
    return NULL;
  }

  if( devReadProbe(1, addr+CR_ASCII_C, &cr[0]) ){
    errlogPrintf("No card in  slot %d\n",slot);
    return NULL;
  }

  cr[1]=*(addr+CR_ASCII_R);
  cr[2]='\0';

  if( cr[0]!='C' || cr[1]!='R' ){
    errlogPrintf("Card in slot %d has non-standard CR layout.  Ignoring...\n",slot);
    return NULL;
  }

  return addr;
}

/*
 * @return 0 Not match
 * @return 1 Match
 */
static
int csrMatch(const struct VMECSRID* A, const struct VMECSRID* B)
{
	if( A->vendor!=B->vendor &&
		A->vendor!=VMECSRANY &&
		B->vendor!=VMECSRANY
	)
		return 0;

	if( A->board!=B->board &&
		A->board!=VMECSRANY &&
		B->board!=VMECSRANY
	)
		return 0;

	if( A->revision!=B->revision &&
		A->revision!=VMECSRANY &&
		B->revision!=VMECSRANY
	)
		return 0;

	return 1;
}

epicsShareFunc
volatile unsigned char* devCSRTestSlot(
	const struct VMECSRID* devs,
	int slot,
	struct VMECSRID* info
)
{
  struct VMECSRID test;
  volatile unsigned char* addr=devCSRProbeSlot(slot);

  if(!addr) return addr;

  test.vendor=CSRRead24(addr + CR_IEEE_OUI);
  test.board=CSRRead32(addr + CR_BOARD_ID);
  test.revision=CSRRead32(addr + CR_REVISION_ID);

  for(; devs && devs->vendor; devs++){
    if(csrMatch(devs,&test)){
      if(!!info){
        info->vendor=test.vendor;
        info->board=test.board;
        info->revision=test.revision;
      }
      return addr;
    }
  }

  return NULL;
}

/* Decode contents of CSR/CR and print to screen.
 *
 * v=0 - basic identification info (vendor/board id)
 * v=1 - config/capability info
 * v=2 - hex dump of start of CR
 */

void vmecsrprint(int N,int v)
{
    volatile unsigned char* addr;
    char ctrlsts=0;
    int i,j,space;
    size_t ader;

    if( N<0 || N>=32 ){
      errlogPrintf("Slot number of of range (1-31)\n");
      return;
    }

    errlogPrintf("====== Slot %d\n",N);

    addr=devCSRProbeSlot(N);
    if(!addr) return;

    if(v>=2){
      for(i=0;i<512;i++){
        if(i%16==0) {
          printf("%04x: ",i);
      }

        printf("%02x", ((int)*(addr+i))&0xff);

        if(i%16==15)
          printf("\n");
        else if(i%4==3)
          printf(" ");
      }
    }

    if(v>=1){
      errlogPrintf("ROM Checksum : 0x%02x\n",CSRRead8(addr + CR_ROM_CHECKSUM));
      errlogPrintf("ROM Length   : 0x%06x\n",CSRRead24(addr + CR_ROM_LENGTH));
      errlogPrintf("CR data width: 0x%02x\n",CSRRead8(addr + CR_DATA_ACCESS_WIDTH));
      errlogPrintf("CSR data width:0x%02x\n",CSRRead8(addr + CSR_DATA_ACCESS_WIDTH));
    }

    space=CSRRead8(addr + CR_SPACE_ID);
    errlogPrintf("CR space id:   ");
    if(space==1)
      errlogPrintf("VME64\n");
    else if(space==2)
      errlogPrintf("VME64x\n");
    else
      errlogPrintf("Unknown (0x%02x)\n",space);

    errlogFlush();

    if(space>=1){
      errlogPrintf("Vendor ID    : 0x%06x\n",CSRRead24(addr + CR_IEEE_OUI));
      errlogPrintf("Board ID     : 0x%08x\n",CSRRead32(addr + CR_BOARD_ID));
      errlogPrintf("Revision ID  : 0x%08x\n",CSRRead32(addr + CR_REVISION_ID));
      errlogPrintf("Program ID   : 0x%02x\n",CSRRead8(addr + CR_PROGRAM_ID));

      errlogPrintf("CSR Bar      : 0x%02x\n",CSRRead8(addr + CSR_BAR));
      ctrlsts=CSRRead8(addr + CSR_BIT_SET);
      errlogPrintf("CSR CS       : 0x%02x\n",ctrlsts);
      errlogPrintf("CSR Reset    : %s\n",ctrlsts&CSR_BITSET_RESET_MODE?"Yes":"No");
      errlogPrintf("CSR Sysfail  : %s\n",ctrlsts&CSR_BITSET_SYSFAIL_ENA?"Yes":"No");
      errlogPrintf("CSR Fail     : %s\n",ctrlsts&CSR_BITSET_MODULE_FAIL?"Yes":"No");
      errlogPrintf("CSR Enabled  : %s\n",ctrlsts&CSR_BITSET_MODULE_ENA?"Yes":"No");
      errlogPrintf("CSR Bus Err  : %s\n",ctrlsts&CSR_BITSET_BERR?"Yes":"No");
    }

    if(space>=2){
      errlogPrintf("User CR      : %08x -> %08x\n",
        CSRRead24(addr + CR_BEG_UCR),CSRRead24(addr + CR_END_UCR));
      errlogPrintf("User CSR     : %08x -> %08x\n",
        CSRRead24(addr + CR_BEG_UCSR),CSRRead24(addr + CR_END_UCSR));
      errlogPrintf("CSR Owned    : %s\n",ctrlsts&CSR_BITSET_CRAM_OWNED?"Yes":"No");
      errlogPrintf("Owner        : 0x%02x\n",CSRRead8(addr + CSR_CRAM_OWNER));
      errlogPrintf("User bits    : 0x%02x\n",CSRRead8(addr + CSR_UD_BIT_SET));
      errlogPrintf("Serial Number: 0x");
      for(i=CR_BEG_SN; i<=CR_END_SN; i+=4)
        errlogPrintf("%02x",CSRRead8(addr + i));
      errlogPrintf("\n");
      if(v>=1){
        errlogFlush();
        errlogPrintf("Master Cap.  : 0x%02x\n",CSRRead16(addr + CR_MASTER_CHAR));
        errlogPrintf("Slave Cap.   : 0x%02x\n",CSRRead16(addr + CR_SLAVE_CHAR));
        errlogPrintf("IRQ Sink Cap.: 0x%02x\n",CSRRead8(addr + CR_IRQ_HANDLER_CAP));
        errlogPrintf("IRQ Src Cap. : 0x%02x\n",CSRRead8(addr + CR_IRQ_CAP));
        errlogPrintf("CRAM data width:0x%02x\n",CSRRead8(addr + CR_CRAM_WIDTH));
        for(i=0;i<8;i++){
          errlogPrintf("Function %d\n",i);
          errlogPrintf("  Data width: %02x\n",CSRRead8(addr + CR_FN_DAWPR(i)));
          errlogPrintf("  Data AM   : ");
          for(j=0;j<0x20;j+=4)
            errlogPrintf("%02x",CSRRead8(addr + CR_FN_AMCAP(i) + j));
          errlogPrintf("\n");
          errlogPrintf("  Data XAM  : ");
          for(j=0;j<0x80;j+=4)
            errlogPrintf("%02x",CSRRead8(addr + CR_FN_XAMCAP(i) + j));
          errlogPrintf("\n");
          errlogPrintf("  Data ADEM : ");
          for(j=0;j<0x10;j+=4)
            errlogPrintf("%02x",CSRRead8(addr + CR_FN_ADEM(i) + j));
          errlogPrintf("\n");
          ader=CSRRead32(addr + CSR_FN_ADER(i));
          errlogPrintf("  Data ADER : Base %08x Mod %02x\n",
                 (unsigned int)ader&0xFfffFf00,(int)(ader&0xff)>>2);
        }
      }
    }

    return;
}


void vmecsrdump(int v)
{
  int i;
  errlogFlush();

  errlogPrintf(">>> CSR/CR Dump\n");

  for(i=0;i<22;i++) {
    vmecsrprint(i,v);
    errlogFlush();
  }

  errlogPrintf(">>> CSR/CR Dump End\n");
  return;
}
