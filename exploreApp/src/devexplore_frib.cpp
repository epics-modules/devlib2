/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 */
// FRIB specific operations

#include <list>
#include <vector>
#include <memory>

#include <stdio.h>

#include <alarm.h>
#include <errlog.h>
#include <devSup.h>
#include <epicsEvent.h>
#include <dbScan.h>
#include <epicsThread.h>
#include <recGbl.h>
#include <waveformRecord.h>
#include <longoutRecord.h>
#include <mbbiRecord.h>
#include <epicsExport.h>

#include "devLibPCI.h"
#include "epicsMMIO.h"
#include "devexplore.h"

namespace {

static const epicsPCIID anypci[] = {
    DEVPCI_DEVICE_VENDOR(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR),
    DEVPCI_END
};

// Read a static ID code 0xF1A54001
// Write 0xC001D00D to enable SPI engine write mode, any other value disables
#define REG_LOCKOUT (0)
// Read ready for new command (mask 0x00000001)
// Write command (mask 0xff000000) and address (mask 0x00ffffff)
#define REG_CMDADDR (4)
// Input for FIFO of 4x 32-bit words used with the write command
#define REG_WDATA   (8)
// Results of a read command
#define REG_RDATA   (12)
#define REGMAX      (16)

struct flashProg : public epicsThreadRunable {
    epicsMutex lock;
    epicsEvent evt;

    const std::string pciname;
    const unsigned bar;
    const epicsPCIDevice *pdev;

    epicsUInt32 pci_offset,   // PCI address of REG_LOCKOUT
                flash_offset, // first address to write in flash chip
                flash_size,   // capacity of flash chip
                flash_last;

    volatile char* pci_base;

    IOSCANPVT scan;

    // we cheat by read and write abort flag w/o locking
    volatile unsigned abort;

    // set before worker starts, read w/o locking in worker
    unsigned debug;

    enum state_t {
        Idle,
        Erase,
        Program,
        Verify,
        Success,
        Fail
    } state;

    std::vector<char> bitfile;

    std::auto_ptr<epicsThread> worker;

    flashProg(const std::string& pname, unsigned bar, epicsUInt32 poffset, epicsUInt32 foffset)
        :pciname(pname), bar(bar), pdev(NULL), pci_offset(poffset)
        ,flash_offset(foffset), flash_size(0), flash_last(0)
        ,abort(0)
        ,state(Idle)
    {
        if(devPCIFindSpec(anypci, pciname.c_str(), &pdev, 0))
            throw std::runtime_error(SB()<<" Invalid PCI device "<<pciname);

        if(devPCIToLocalAddr(pdev, bar, (volatile void**)&pci_base, 0))
            throw std::runtime_error(SB()<<" Failed to map bar "<<bar<<" of "<<pciname);
        pci_base += pci_offset;
        epicsUInt32 barsize = 0;
        if(devPCIBarLen(pdev, bar, &barsize))
            throw std::runtime_error(SB()<<" Failed to find size of bar "<<bar);
        if(barsize<pci_offset+REGMAX)
            throw std::runtime_error(SB()<<"PCI offset + REGMAX exceeds BAR "<<bar<<" size");

        epicsUInt32 id = read32(REG_LOCKOUT);
        if(id!=0xF1A54001)
            throw std::runtime_error(SB()<<"wrong id 0x"<<std::hex<<id<<" from 0x"<<std::hex<<(pci_base+REG_LOCKOUT));

        scanIoInit(&scan);
    }

    void write32(unsigned offset, epicsUInt32 val) {
        if(debug>2)
            printf("Write %x <- %08x\n", pci_offset+offset, (unsigned)val);
        le_iowrite32(pci_base+offset, val);
    }

    epicsUInt32 read32(unsigned offset) {
        epicsUInt32 ret = le_ioread32(pci_base+offset);
        if(debug>2)
            printf("Read %x -> %08x\n", pci_offset+offset, (unsigned)ret);
        return ret;
    }

    void wait_for_ready() {
        bool ready;
        do{
            ready = read32(REG_CMDADDR)&1;
        } while(!ready && !abort);
    }

    void wait_for_ready(double sleep) {
        bool ready;
        do{
            evt.wait(sleep);
            ready = read32(REG_CMDADDR)&1;
        } while(!ready && !abort);
    }

    virtual void run()
    {
        epicsUInt32 lastaddr = 0;
        Guard G(lock);
        try {
            if(bitfile.empty())
                throw std::runtime_error("No image");
            if(bitfile.size()+flash_offset>flash_size)
                throw std::runtime_error("image size+offset exceeds capacity");
            if(flash_offset&0xffff)
                throw std::runtime_error("offset not aligned to 64k");

            if(debug>1) errlogPrintf("flash offset=%x size=%x\n", (unsigned)flash_offset, (unsigned)flash_size);
            if(flash_offset>=0x1000000 || flash_size>0x1000000)
                throw std::runtime_error("Flash addresses must be 24-bit");

            std::vector<char> file;
            file.swap(bitfile); // consume image

            // zero pad to 16 byte boundary (we write only in 16 byte blocks)
            file.resize( ((file.size()-1u)|0xf)+1u, 0 );
            assert((file.size()%16u==0));

            const epicsUInt32 fstart = flash_offset,
                              fend  = flash_offset + std::min(file.size(), (size_t)flash_size);

            epicsUInt32 id = read32(REG_LOCKOUT);
            if(id!=0xF1A54001)
                throw std::runtime_error(SB()<<"wrong id 0x"<<std::hex<<id<<" from 0x"<<std::hex<<(pci_base+REG_LOCKOUT));

            if(debug)
                errlogPrintf("Will program %x -> %x\n", (unsigned)fstart, (unsigned)fend);

            // unlock write logic
            write32(REG_LOCKOUT, 0xC001D00D);

            // erase in 64k blocks
            {
                state = Erase;
                UnGuard U(G);
                scanIoRequest(scan);

                for(lastaddr = fstart; lastaddr<fend && !abort; lastaddr+=0x10000) {

                    write32(REG_CMDADDR, 0x06000000); // write enable
                    wait_for_ready();
                    write32(REG_CMDADDR, 0xD8000000|lastaddr); // block erase (64k)

                    // 64k block erase time is spec'd at 150ms typical, 2000ms max
                    wait_for_ready(0.05);
                }
            }

            if(abort)
                throw std::runtime_error("Abort Erase");

            // program in 16 byte blocks
            {
                state = Program;
                UnGuard U(G);
                scanIoRequest(scan);

                epicsUInt32 ioffset = 0;
                for(lastaddr = fstart; lastaddr<fend && !abort; lastaddr+=16, ioffset += 16) {
                    const epicsUInt32 *data = (const epicsUInt32 *)&file[ioffset];

                    write32(REG_CMDADDR, 0x06000000); // write enable

                    write32(REG_WDATA, ntohl(data[3]));
                    write32(REG_WDATA, ntohl(data[2]));
                    write32(REG_WDATA, ntohl(data[1]));
                    write32(REG_WDATA, ntohl(data[0]));
                    wait_for_ready();

                    write32(REG_CMDADDR, 0x02000000|lastaddr);

                    // page program time is speced at 0.7ms typical, 3ms max
                    // however, this is for the whole page
                    wait_for_ready();
                }
            }

            if(abort)
                throw std::runtime_error("Abort Program");

            // Verify in 4 byte blocks
            {
                state = Verify;
                UnGuard U(G);
                scanIoRequest(scan);

                epicsUInt32 ioffset = 0;
                for(lastaddr = fstart; lastaddr<fend && !abort; lastaddr+=4, ioffset += 4) {
                    const epicsUInt32 expect = ntohl(*(const epicsUInt32*)&file[ioffset]);

                    write32(REG_CMDADDR, 0x03000000|lastaddr);
                    wait_for_ready();

                    epicsUInt32 actual = read32(REG_RDATA);

                    if(actual!=expect)
                        throw std::runtime_error(SB()<<"Verify mis-match 0x"
                                                 <<std::hex<<actual
                                                 <<" != 0x"
                                                 <<std::hex<<expect);
                }
            }

            if(abort)
                throw std::runtime_error("Abort Verify");

            state = Success;
            flash_last = 0;

        }catch(std::exception& e){
            errlogPrintf("Exception: %s\n", e.what());
            state = Fail;
            flash_last = lastaddr;
        }

        // re-lock write logic
        write32(REG_LOCKOUT, 0x00000000);
        scanIoRequest(scan);

        abort = 0;

        worker.reset();
        if(debug) errlogPrintf("Worker exits\n");
    }
};

typedef std::list<flashProg*> progs_t;
progs_t progs;

long init_record_common(dbCommon *prec)
{
    try {
        DBEntry ent(prec);

        DBLINK *plink = ent.getDevLink();
        assert(plink && plink->type==INST_IO);

        std::string lstr(plink->value.instio.string);

        size_t sep(lstr.find_first_of(" \t"));
        if(sep>=lstr.size())
            throw std::runtime_error(SB()<<"Missing expected space in INP/OUT \""<<lstr<<"\"");

        // required
        std::string pciname(lstr.substr(0, sep));
        epicsUInt32 pci_offset, flash_offset;
        unsigned bar = 0;

        strmap_t args;
        parseToMap(lstr.substr(sep), args);

        strmap_t::const_iterator it;

        if((it=args.find("pci_offset"))!=args.end()) {
            pci_offset = parseU32(it->second);
        } else {
            throw std::runtime_error(SB()<<"Missing required 'pci_offset' in \""<<lstr<<"\"");
        }
        if((it=args.find("flash_offset"))!=args.end()) {
            flash_offset = parseU32(it->second);
        } else {
            throw std::runtime_error(SB()<<"Missing required 'flash_offset' in \""<<lstr<<"\"");
        }

        if((it=args.find("bar"))!=args.end()) {
            bar = parseU32(it->second);
        }

        if(prec->tpro)
            fprintf(stderr, "%s: pcidev=%s offset=%x\n", prec->name, pciname.c_str(), (unsigned)pci_offset);

        flashProg *priv = NULL;

        for(progs_t::const_iterator it = progs.begin(), end = progs.end();
            it != end; ++it)
        {
            flashProg *F = *it;
            if(F->pciname==pciname && F->bar==bar && F->pci_offset==pci_offset && F->flash_offset==flash_offset) {
                priv = F;
                break;
            }
        }
        if(!priv) {
            priv = new flashProg(pciname, bar, pci_offset, flash_offset);
            progs.push_back(priv);
        }
        prec->dpvt = priv;

        if((it=args.find("flash_size"))!=args.end()) {
            priv->flash_size = parseU32(it->second);
        }

    } catch(std::exception& e) {
        fprintf(stderr, "%s: init_record error: %s\n", prec->name, e.what());
    }
    return 0;
}

long load_bitfile_wf(waveformRecord *prec)
{
    flashProg *priv = (flashProg*)prec->dpvt;
    if(!priv) {
        (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
    try {
        std::vector<char> buf(prec->nord);
        const char *ibuf = (const char*)prec->bptr;

        std::copy(ibuf, ibuf+buf.size(), buf.begin());

        Guard G(priv->lock);

        priv->bitfile.swap(buf);

        return 0;
    } catch(std::exception& e) {
        fprintf(stderr, "%s: load_bitfile_wf error: %s\n", prec->name, e.what());
        (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
}

long startstop_lo(longoutRecord *prec)
{
    flashProg *priv = (flashProg*)prec->dpvt;
    if(!priv) {
        (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
    try {

        Guard G(priv->lock);

        if(prec->val && !priv->worker.get()) {
            if(prec->tpro>1)
                errlogPrintf("%s: start programming\n", prec->name);
            priv->debug = prec->tpro;
            priv->worker.reset(new epicsThread(*priv, "flasher",
                                               epicsThreadGetStackSize(epicsThreadStackSmall),
                                               epicsThreadPriorityScanLow+1));
            priv->worker->start();

        } else if(!prec->val && priv->worker.get()) {
            if(prec->tpro>1)
                errlogPrintf("%s: abort programming\n", prec->name);
            priv->abort = 1;
            priv->evt.signal();
        }

        return 0;
    } catch(std::exception& e) {
        fprintf(stderr, "%s: load_bitfile_wf error: %s\n", prec->name, e.what());
        (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
}

long status_mbbi(mbbiRecord *prec)
{
    flashProg *priv = (flashProg*)prec->dpvt;
    if(!priv) {
        (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
    try {

        Guard G(priv->lock);

        prec->rval = (int)priv->state;

        return 0;
    } catch(std::exception& e) {
        fprintf(stderr, "%s: load_bitfile_wf error: %s\n", prec->name, e.what());
        (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return S_dev_noDevice;
    }
}

long status_get_iointr_info(int dir, dbCommon* prec, IOSCANPVT* ppscan)
{
    flashProg *priv = (flashProg*)prec->dpvt;
    if(priv) {
        *ppscan = priv->scan;
    }
    return 0;
}

struct dset6 {
    dset base;
    DEVSUPFUN read;
    DEVSUPFUN junk;
};
#define DSET(NAME, INITREC, IOINTR, RW) static dset6 NAME = {{6, NULL, NULL, (DEVSUPFUN)INITREC, (DEVSUPFUN)IOINTR}, (DEVSUPFUN)RW}; \
    epicsExportAddress(dset, NAME)

} // namespace

DSET(devExploreFRIBFlashWf,   &init_record_common, NULL, &load_bitfile_wf);
DSET(devExploreFRIBFlashLo,   &init_record_common, NULL, &startstop_lo);
DSET(devExploreFRIBFlashMbbi, &init_record_common, &status_get_iointr_info, &status_mbbi);
