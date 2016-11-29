/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <set>

#include <string.h>
#include <errno.h>

#include <epicsVersion.h>
#include <epicsStdlib.h>
#include <devSup.h>
#include <drvSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <callback.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <menuFtype.h>
#include <epicsExit.h>
#include <cantProceed.h>
#include <ellLib.h>
#include <iocsh.h>
#include <dbScan.h>
#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <epicsExit.h>

#include <longinRecord.h>
#include <devSup.h>
#include <epicsExport.h>

#include <epicsMMIO.h>

#include "devLibPCI.h"

#ifdef EPICS_VERSION_INT
#if EPICS_VERSION_INT>=VERSION_INT(3,15,0,1)
#  define USE_COMPLETE
#endif
#endif

#ifndef __rtems__
#define printk errlogPrintf
#endif

namespace {

typedef epicsGuard<epicsMutex> Guard;

std::set<epicsUInt32> irq_used;

struct priv {
    const epicsPCIDevice *dev;
    epicsMutex lock;
    IOSCANPVT scan;
    unsigned wait_for;
    bool queued, scan_queued;
    unsigned errd;
    epicsUInt32 irqs, lost;

#ifndef USE_COMPLETE
    CALLBACK waiters[NUM_CALLBACK_PRIORITIES];
#endif
    priv() :dev(NULL), wait_for(0),
        queued(false), scan_queued(true), errd(0),
        irqs(0), lost(0) {
        scanIoInit(&scan);
    }

    void doscan() {
#ifdef USE_COMPLETE
        wait_for = scanIoRequest(scan);
#else
        scanIoRequest(scan);
        wait_for = (1<<NUM_CALLBACK_PRIORITIES)-1;
        for(unsigned i=0; i<NUM_CALLBACK_PRIORITIES; i++)
            callbackRequest(&waiters[i]);
#endif
    }
};

static const epicsPCIID anypci[] = {
    DEVPCI_DEVICE_VENDOR(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR),
    DEVPCI_END
};

static
void isrfn(void *raw)
{
    priv *pvt = (priv*)raw;
    try {
        Guard G(pvt->lock);
        if(pvt->wait_for) {
            pvt->lost++;
            pvt->queued = 1;
        } else {
            pvt->irqs++;
            pvt->doscan();
        }
        if(pvt->errd)
            printk("Error in ISRFN %x:%x.%x Clears\n",
                   pvt->dev->bus,
                   pvt->dev->device,
                   pvt->dev->function);
        pvt->errd = 0;
    } catch(std::exception& e) {
        if(!pvt->errd)
            printk("Error in ISRFN %x:%x.%x: %s\n",
                   pvt->dev->bus,
                   pvt->dev->device,
                   pvt->dev->function,
                   e.what());
        pvt->errd = 1;
    }
}

static
#ifdef USE_COMPLETE
void irq_scan_complete(void *usr, IOSCANPVT scan, int prio)
{
    priv *pvt = (priv*)usr;
#else
void irq_scan_complete(CALLBACK* pcb)
{
    priv *pvt = (priv*)pcb->user;
    int prio = pcb->priority;
#endif
    try {
        Guard G(pvt->lock);
        if(pvt->wait_for==0)
            errlogPrintf("Extra callback for %x:%x.%x\n",
                         pvt->dev->bus,
                         pvt->dev->device,
                         pvt->dev->function);
        pvt->wait_for &= ~(1<<prio);
        if(pvt->scan_queued && pvt->wait_for==0 && pvt->queued) {
            pvt->queued = 0;
            pvt->doscan();
        }
    } catch(std::exception& e) {
        pvt->errd = 1;
        errlogPrintf("Error in irq_scan_complete %x:%x.%x: %s\n",
                     pvt->dev->bus,
                     pvt->dev->device,
                     pvt->dev->function,
                     e.what());
    }
}

static
void isr_stop(void *raw)
{
    priv *pvt = (priv*)raw;
    devPCIDisconnectInterrupt(pvt->dev, &isrfn, raw);
}

static
long init_record_li_irq(longinRecord *prec)
{
    try {
        std::auto_ptr<priv> pvt(new priv);

        if(devPCIFindSpec(anypci,
                          prec->inp.value.instio.string,
                          &pvt->dev,
                          0))
            throw std::runtime_error("Failed to match PCI device");

        epicsUInt32 bdf = pvt->dev->bus<<16
                        | pvt->dev->device<<8
                        | pvt->dev->function;

        if(prec->tpro>1)
            printf("%s: matched %x:%x.%x %s\n",
                   prec->name,
                   pvt->dev->bus,
                   pvt->dev->device,
                   pvt->dev->function,
                   pvt->dev->slot);

        if(irq_used.find(bdf)!=irq_used.end())
            throw std::runtime_error("IRQ already used by another record");

        irq_used.insert(bdf);

        if(devPCIConnectInterrupt(pvt->dev, &isrfn, pvt.get(), 0))
            throw std::runtime_error("Failed to Connect IRQ");

        if(devPCIEnableInterrupt(pvt->dev))
            throw std::runtime_error("Failed to Enable IRQ");

#ifdef USE_COMPLETE
        scanIoSetComplete(pvt->scan, irq_scan_complete, pvt.get());
#else
        for(unsigned i=0; i<NUM_CALLBACK_PRIORITIES; i++) {
            callbackSetPriority(i, &pvt->waiters[i]);
            callbackSetCallback(irq_scan_complete, &pvt->waiters[i]);
            callbackSetUser(pvt.get(), &pvt->waiters[i]);
        }
#endif

        prec->dpvt = pvt.release();

        epicsAtExit(isr_stop, prec->dpvt);
        return 0;

    } catch(std::exception &e) {
        printf("%s: init error: %s\n", prec->name, e.what());
        return EINVAL;
    }
}

static
long get_io_intr_irq(int dir, dbCommon* prec, IOSCANPVT* ppscan)
{
    priv *pvt = (priv*)prec->dpvt;
    if (pvt)
        *ppscan = pvt->scan;
    return 0;
}

static
long read_irq(longinRecord *prec)
{
    priv *pvt = (priv*)prec->dpvt;
    if (pvt) {
        Guard G(pvt->lock);
        prec->val = pvt->irqs;
        if (prec->tpro > 1 && pvt->lost) {
            errlogPrintf("%s: lost %u IRQs\n", prec->name, (unsigned)pvt->lost);
            pvt->lost = 0;
        }
        return 0;
    } else {
        (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
        return EINVAL;
    }
}

} //namespace

static struct dset5 {
    dset base;
    DEVSUPFUN read;
} devExploreLiIRQ = {
    {5, NULL, NULL,
        (DEVSUPFUN)&init_record_li_irq,
        (DEVSUPFUN)&get_io_intr_irq,
    },
    (DEVSUPFUN)&read_irq,
};

extern "C" {
epicsExportAddress(dset, devExploreLiIRQ);
}
