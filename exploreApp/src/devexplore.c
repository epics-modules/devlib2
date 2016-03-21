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
#include <ellLib.h>
#include <iocsh.h>

#include <dbCommon.h>
#include <longoutRecord.h>
#include <longinRecord.h>
#include <mbbiDirectRecord.h>
#include <mbbiRecord.h>
#include <biRecord.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <waveformRecord.h>
#include <devSup.h>
#include <epicsExport.h>

#include <epicsMMIO.h>

#include "devLibPCI.h"

int pciexploredebug=1;

static const epicsPCIID anypci[] = {
    DEVPCI_DEVICE_VENDOR(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR),
    DEVPCI_END
};

typedef struct {
    ELLNODE node;
    epicsMutexId mutex;
    volatile char *base;
} device;

static
ELLLIST explorepcis;

typedef struct {
    const char *addr;
    unsigned bar;
    unsigned offset;

    const epicsPCIDevice *pcidev;
    volatile char *base;
    device *dev;
} priv;

static
epicsUInt32 priv_read32(priv *P, dbCommon *prec)
{
    epicsUInt32 ret = le_ioread32(P->base+P->offset);
    if(pciexploredebug>1) fprintf(stderr, "%s: %x -> %x\n", prec->name, P->offset, (unsigned)ret);
    return ret;
}

static
void priv_write32(priv *P, dbCommon *prec, epicsUInt32 val)
{
    if(pciexploredebug>1) fprintf(stderr, "%s: %x <- %x\n", prec->name, P->offset, (unsigned)val);
    le_iowrite32(P->base+P->offset, val);
}

static
device *getDev(volatile char *base)
{
    device *ret;
    ELLNODE *cur;

    for(cur=ellFirst(&explorepcis); cur; cur=ellNext(cur))
    {
        device *dev = CONTAINER(cur, device, node);
        if(dev->base==base)
            return dev;
    }

    ret = callocMustSucceed(1, sizeof(*ret), "explore getDev");
    ret->base = base;
    ret->mutex = epicsMutexMustCreate();
    ellAdd(&explorepcis, &ret->node);
    return ret;
}

static
int procLink(dbCommon *prec, priv *P, const char *link)
{
    unsigned DM=0, B=0, D=0, F=0, extra=0;
    int ret;
    epicsUInt32 len;

    P->bar = P->offset = 0;

    ret = sscanf(link, "%x:%x:%x.%x %x %x %x", &DM, &B, &D, &F, &P->bar, &P->offset, &extra);
    if(ret>=4) goto findcard;

    DM = B = D = F = extra = 0;

    ret = sscanf(link, "%x:%x.%x %x %x %x", &B, &D, &F, &P->bar, &P->offset, &extra);
    if(ret>=3) goto findcard;

    DM = B = D = F = extra = 0;

    ret = sscanf(link, "%x:%x %x %x %x", &B, &D, &P->bar, &P->offset, &extra);
    if(ret>=2) goto findcard;

    printf("%s: Invalid link \"%s\"\n", prec->name, link);
    return -1;

findcard:
    P->offset += extra;

    if(devPCIFindDBDF(anypci, DM, B, D, F, &P->pcidev, 0)) {
        printf("%s: no device %x:%x:%x.%x\n", prec->name, DM, B, D, F);
        return -1;
    }

    if(devPCIToLocalAddr(P->pcidev, P->bar, (volatile void **)&P->base, 0)) {
        printf("%s: %x:%x:%x.%x failed to map bar %u\n", prec->name, DM, B, D, F, P->bar);
        return -1;
    }

    if(devPCIBarLen(P->pcidev, P->bar, &len)==0 && len<=P->offset) {
        printf("%s: %x:%x:%x.%x base %x out of range for bar %u\n", prec->name, DM, B, D, F, P->offset, P->bar);
        return -1;
    }

    if(pciexploredebug>0)
        printf("%s: %x:%x:%x.%x %x %x\n", prec->name, DM, B, D, F, P->bar, P->offset);
    return 0;
}

static
long init_record_ai(aiRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->inp.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->inp.value.instio.string)) {
        free(P);
        return 0;
    }
    P->dev = getDev(P->base);

    prec->dpvt = P;
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
    P->dev = getDev(P->base);

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
    P->dev = getDev(P->base);

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
    P->dev = getDev(P->base);

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
    P->dev = getDev(P->base);

    prec->dpvt = P;
    return 0;
}

static
long init_record_ao(aoRecord *prec)
{
    priv *P = callocMustSucceed(1, sizeof(*P), "explore priv");

    assert(prec->out.type==INST_IO);

    if(procLink((dbCommon*)prec, P, prec->out.value.instio.string)) {
        free(P);
        return 0;
    }
    P->dev = getDev(P->base);

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
    P->dev = getDev(P->base);

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
    P->dev = getDev(P->base);

    prec->dpvt = P;
    return 0;
}

static
long read_ai(aiRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    prec->rval = priv_read32(P, (dbCommon*)prec);
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long read_li(longinRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    prec->val = priv_read32(P, (dbCommon*)prec);
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long read_mbbidirect(mbbiDirectRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    prec->val = priv_read32(P, (dbCommon*)prec);
    prec->val >>= prec->shft;
    if(prec->mask) prec->val &= prec->mask;
    epicsMutexUnlock(P->dev->mutex);
    return 2;
}

static
long read_mbbi(mbbiRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    prec->val = priv_read32(P, (dbCommon*)prec) & prec->mask;
    prec->val >>= prec->shft;
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long read_bi(biRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    prec->rval = priv_read32(P, (dbCommon*)prec) & prec->mask;
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long write_ao(aoRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    priv_write32(P, (dbCommon*)prec, prec->rval);
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long write_lo(longoutRecord *prec)
{
    priv *P = prec->dpvt;
    if(!P) return 0;

    epicsMutexMustLock(P->dev->mutex);
    priv_write32(P, (dbCommon*)prec, prec->val);
    prec->val = le_ioread32(P->base+P->offset);
    epicsMutexUnlock(P->dev->mutex);
    return 0;
}

static
long read_wf(waveformRecord *prec)
{
    epicsUInt32 *buf = prec->bptr, cnt = prec->nelm, *addr, i;
    priv *P = prec->dpvt;
    if(!P) return 0;

    addr = (epicsUInt32*)(P->base+P->offset);

    epicsMutexMustLock(P->dev->mutex);
    for(i=cnt; i; i--, buf++, addr++) {
        *buf = le_ioread32(addr);
    }
    epicsMutexUnlock(P->dev->mutex);
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

DSET(devExplorePCIReadAI, init_record_ai, NULL, read_ai);
DSET(devExplorePCIReadLI, init_record_li, NULL, read_li);
DSET(devExplorePCIReadMBBIDIRECT, init_record_mbbidirect, NULL, read_mbbidirect);
DSET(devExplorePCIReadMBBI, init_record_mbbi, NULL, read_mbbi);
DSET(devExplorePCIReadBI, init_record_bi, NULL, read_bi);
DSET(devExplorePCIReadWF, init_record_wf, NULL, read_wf);

DSET(devExplorePCIWriteLO, init_record_lo, NULL, write_lo);
DSET(devExplorePCIWriteAO, init_record_ao, NULL, write_ao);
