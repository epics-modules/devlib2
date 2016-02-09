/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */

#include <stdlib.h>
#include <stdio.h>

#include <devSup.h>
#include <drvSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <menuFtype.h>
#include <epicsExit.h>
#include <cantProceed.h>
#include <iocsh.h>

#include <dbCommon.h>
#include <longoutRecord.h>
#include <longinRecord.h>
#include <mbbiDirectRecord.h>
#include <mbbiRecord.h>
#include <biRecord.h>
#include <waveformRecord.h>
#include <devSup.h>
#include <epicsExport.h>

#include <epicsMMIO.h>

#include "devLibPCI.h"

static const epicsPCIID anypci[] = {
    DEVPCI_DEVICE_VENDOR(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR),
    DEVPCI_END
};

typedef struct {
    const char *addr;
    unsigned bar;
    unsigned offset;

    const epicsPCIDevice *dev;
    volatile char *base;
} priv;

static
int procLink(dbCommon *prec, priv *P, const char *link)
{
    unsigned DM=0, B=0, D=0, F=0;
    int ret;
    epicsUInt32 len;

    P->bar = P->offset = 0;

    ret = sscanf(link, "%x:%x:%x.%x %x %x", &DM, &B, &D, &F, &P->bar, &P->offset);
    if(ret>=4) goto findcard;

    DM = B = D = F = 0;

    ret = sscanf(link, "%x:%x.%x %x %x", &B, &D, &F, &P->bar, &P->offset);
    if(ret>=3) goto findcard;

    DM = B = D = F = 0;

    ret = sscanf(link, "%x:%x %x %x", &B, &D, &P->bar, &P->offset);
    if(ret>=2) goto findcard;

    printf("%s: Invalid link \"%s\"\n", prec->name, link);
    return -1;

findcard:
    if(devPCIFindDBDF(anypci, DM, B, D, F, &P->dev, 0)) {
        printf("%s: no device %x:%x:%x.%x\n", prec->name, DM, B, D, F);
        return -1;
    }

    if(devPCIToLocalAddr(P->dev, P->bar, (volatile void **)&P->base, 0)) {
        printf("%s: %x:%x:%x.%x failed to map bar %u\n", prec->name, DM, B, D, F, P->bar);
        return -1;
    }

    if(devPCIBarLen(P->dev, P->bar, &len)==0 && len<=P->offset) {
        printf("%s: %x:%x:%x.%x base %x out of range for bar %u\n", prec->name, DM, B, D, F, P->offset, P->bar);
        return -1;
    }

    return 0;
}

static
long init_record_li(longinRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long init_record_mbbidirect(mbbiDirectRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long init_record_mbbi(mbbiRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long init_record_bi(biRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long init_record_lo(longoutRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->out.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->out.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long init_record_wf(waveformRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);
    if(prec->ftvl!=menuFtypeULONG) {
        printf("%s: invalid FTVL (must be ULONG)\n", prec->name);
        free(P);
        return 0;
    }

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }

    prec->dpvt = P;
    return 0;
}

static
long read_li(longinRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    prec->val = le_ioread32(P->base+P->offset);
    return 0;
}

static
long read_mbbidirect(mbbiDirectRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    prec->val = le_ioread32(P->base+P->offset) & prec->mask;
    prec->val >>= prec->shft;
    return 0;
}

static
long read_mbbi(mbbiRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    prec->val = le_ioread32(P->base+P->offset) & prec->mask;
    return 0;
}

static
long read_bi(mbbiRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    prec->val = le_ioread32(P->base+P->offset) & prec->mask;
    return 0;
}

static
long write_lo(longoutRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    le_iowrite32(P->base+P->offset, prec->val);
    prec->val = le_ioread32(P->base+P->offset);
    return 0;
}

static
long read_wf(waveformRecord *prec)
{
    epicsUInt32 *buf = prec->bptr, cnt = prec->nelm, *addr;
    priv *P = prec->dpvt;
    if(!P) return 0;

    addr = (epicsUInt32*)(P->base+P->offset);

    for(; cnt; cnt--, buf++, addr++) {
        *buf = le_ioread32(addr);
    }
    prec->nord = cnt;
    return 0;
}

typedef struct {
    long N;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_io_intr_info;
    DEVSUPFUN R;
    DEVSUPFUN linconv;
} dset6;

#define DSET(NAME, INITREC, IOINTR, RW) static dset6 NAME = \
    {6, NULL, NULL, INITREC, IOINTR, RW, NULL}; \
epicsExportAddress(dset, NAME)

DSET(devExplorePCIReadLI, init_record_li, NULL, read_li);
DSET(devExplorePCIReadMBBIDIRECT, init_record_mbbidirect, NULL, read_mbbidirect);
DSET(devExplorePCIReadMBBI, init_record_mbbi, NULL, read_mbbi);
DSET(devExplorePCIReadBI, init_record_bi, NULL, read_bi);
DSET(devExplorePCIReadWF, init_record_wf, NULL, read_wf);

DSET(devExplorePCIWriteLO, init_record_lo, NULL, write_lo);
