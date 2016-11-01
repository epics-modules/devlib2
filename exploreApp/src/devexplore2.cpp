/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>

#include <string.h>
#include <errno.h>

#include <epicsVersion.h>
#include <epicsStdlib.h>
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
#include <dbScan.h>
#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>

#include <dbCommon.h>
#include <longoutRecord.h>
#include <longinRecord.h>
#include <mbboDirectRecord.h>
#include <mbbiDirectRecord.h>
#include <mbboRecord.h>
#include <mbbiRecord.h>
#include <biRecord.h>
#include <aiRecord.h>
#include <boRecord.h>
#include <aoRecord.h>
#include <waveformRecord.h>
#include <devSup.h>
#include <epicsExport.h>

#include <epicsMMIO.h>

#include "devLibPCI.h"

#if EPICS_REVISION<=14

#define M_stdlib 4242
#define S_stdlib_noConversion (M_stdlib | 1) /* No digits to convert */
#define S_stdlib_extraneous   (M_stdlib | 2) /* Extraneous characters */
#define S_stdlib_underflow    (M_stdlib | 3) /* Too small to represent */
#define S_stdlib_overflow     (M_stdlib | 4) /* Too large to represent */
#define S_stdlib_badBase      (M_stdlib | 5) /* Number base not supported */

epicsShareFunc int
epicsParseULong(const char *str, unsigned long *to, int base, char **units)
{
    int c;
    char *endp;
    unsigned long value;

    while ((c = *str) && isspace(c))
        ++str;

    errno = 0;
    value = strtoul(str, &endp, base);

    if (endp == str)
        return S_stdlib_noConversion;
    if (errno == EINVAL)    /* Not universally supported */
        return S_stdlib_badBase;
    if (errno == ERANGE)
        return S_stdlib_overflow;

    while ((c = *endp) && isspace(c))
        ++endp;
    if (c && !units)
        return S_stdlib_extraneous;

    *to = value;
    if (units)
        *units = endp;
    return 0;
}

static int
epicsParseUInt32(const char *str, epicsUInt32 *to, int base, char **units)
{
    unsigned long value;
    int status = epicsParseULong(str, &value, base, units);

    if (status)
        return status;

#if (ULONG_MAX > 0xffffffffULL)
    if (value > 0xffffffffUL && value <= ~0xffffffffUL)
        return S_stdlib_overflow;
#endif

    *to = (epicsUInt32) value;
    return 0;
}
#endif


extern
volatile void * const exploreTestBase;
extern
const epicsUInt32 exploreTestSize;


static
volatile epicsUInt32 exploreTestRegion[1024];

volatile void * const exploreTestBase = (volatile void*)exploreTestRegion;
const epicsUInt32 exploreTestSize = sizeof(exploreTestRegion);

namespace {

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

struct SB {
    std::ostringstream strm;
    operator std::string() { return strm.str(); }
    template<typename T>
    SB& operator<<(const T& v) {
        strm<<v;
        return *this;
    }
};

epicsUInt32 parseU32(const std::string& s)
{
    epicsUInt32 ret;
    int err = epicsParseUInt32(s.c_str(), &ret, 0, NULL);
    if(err) {
        char msg[80];
        errSymLookup(err, msg, sizeof(msg));
        throw std::runtime_error(SB()<<"Error parsing '"<<s<<"' : "<<msg);
    }
    return ret;
}

union punny32 {
    epicsUInt32 ival;
    epicsFloat32 fval;
};

// value conversion
//  cast different integer sizes using C rules
//  type pun int to/from float
template<typename TO, typename FROM>
struct castval { static TO op(FROM v) { return v; } };
template<typename FROM>
struct castval<epicsFloat32,FROM> { static epicsFloat32 op(FROM v) {punny32 P; P.ival = v; return P.fval;} };
template<typename TO>
struct castval<TO,epicsFloat32> { static TO op(epicsFloat32 v) {punny32 P; P.fval = v; return P.ival;} };

struct priv {

    epicsMutex lock;

    std::string pciname;
    unsigned bar;
    // offset of first element within BAR
    epicsUInt32 offset;
    // step between elements (waveform only)
    epicsInt32 step;

    // size of a single value
    unsigned valsize;
    // endianness of value
    enum ORD {
        NAT,
        BE,
        LE
    } ord;

    unsigned vshift;
    epicsUInt32 vmask;

    volatile void *base;
    epicsUInt32 barsize;

    bool initread;

    priv() :bar(0u), offset(0u), step(0), valsize(1), ord(NAT), vshift(0u), vmask(0u), initread(false) {}

    template<typename VAL>
    VAL readraw(epicsUInt32 off=0) const
    {
        volatile char *addr = (volatile char*)base+offset+off;
        epicsUInt32 OV = -1;
        switch(valsize) {
        case 1: OV = ioread8(addr); break;
        case 2: switch(ord) {
            case NAT: OV = nat_ioread16(addr); break;
            case BE:  OV = be_ioread16(addr); break;
            case LE:  OV = le_ioread16(addr); break;
            }
            break;
        case 4: switch(ord) {
            case NAT: OV = nat_ioread32(addr); break;
            case BE:  OV = be_ioread32(addr); break;
            case LE:  OV = le_ioread32(addr); break;
            }
            break;
        }
        return OV;
    }

    template<typename VAL>
    VAL read(epicsUInt32 off=0) const
    {
        epicsUInt32 OV(readraw<VAL>(off));
        if(vmask) OV &= vmask;
        OV >>= vshift;
        return OV;
    }

    template<typename VAL>
    unsigned readArray(VAL *val, unsigned count) const
    {
        epicsUInt32 addr = 0,
                    end  = barsize-offset;
        unsigned i;
        for(i=0; i<count && addr<end && addr>=0; i++, addr+=step)
        {
            epicsUInt32 OV = read<VAL>(addr);
            *val++ = castval<VAL,epicsUInt32>::op(OV);
        }
        return i;
    }

    template<typename VAL>
    void write(VAL val, epicsUInt32 off=0)
    {
        volatile char *addr = (volatile char*)base+offset+off;
        epicsUInt32 V = castval<epicsUInt32,VAL>::op(val)<<vshift;

        if(vmask) {
            // Do RMW
            V &= vmask;
            V |= readraw<epicsUInt32>(off)&(~vmask);
        }

        switch(valsize) {
        case 1: iowrite8(addr, V); break;
        case 2: switch(ord) {
            case NAT: nat_iowrite16(addr, V); break;
            case BE:  be_iowrite16(addr, V); break;
            case LE:  le_iowrite16(addr, V); break;
            }
            break;
        case 4: switch(ord) {
            case NAT: nat_iowrite32(addr, V); break;
            case BE:  be_iowrite32(addr, V); break;
            case LE:  le_iowrite32(addr, V); break;
            }
            break;
        }
    }

    template<typename VAL>
    unsigned writeArray(const VAL *val, unsigned count)
    {
        epicsUInt32 addr = 0,
                    end  = barsize-offset;

        unsigned i;
        for(i=0; i<count && addr<end && addr>=0; i++, addr+=step)
        {
            write(*val++, addr);
        }
        return i;
    }
};

class DBEntry {
    DBENTRY entry;
public:
    DBENTRY *pentry() const { return const_cast<DBENTRY*>(&entry); }
    DBEntry(dbCommon *prec) {
        dbInitEntry(pdbbase, &entry);
        if(dbFindRecord(&entry, prec->name))
            throw std::logic_error(SB()<<"getLink can't find record "<<prec->name);
    }
    DBEntry(const DBEntry& ent) {
        dbCopyEntryContents(const_cast<DBENTRY*>(&ent.entry), &entry);
    }
    DBEntry& operator=(const DBEntry& ent) {
        dbFinishEntry(&entry);
        dbCopyEntryContents(const_cast<DBENTRY*>(&ent.entry), &entry);
        return *this;
    }
    ~DBEntry() {
        dbFinishEntry(&entry);
    }
    DBLINK *getDevLink() const {
        if(dbFindField(pentry(), "INP") && dbFindField(pentry(), "OUT"))
            throw std::logic_error(SB()<<entry.precnode->recordname<<" has no INP/OUT?!?!");
        if(entry.pflddes->field_type!=DBF_INLINK &&
           entry.pflddes->field_type!=DBF_OUTLINK)
            throw std::logic_error(SB()<<entry.precnode->recordname<<" not devlink or IN/OUT?!?!");
        return (DBLINK*)entry.pfield;
    }
    const char *info(const char *name, const char *def) const
    {
        if(dbFindInfo(pentry(), name))
            return def;
        else
            return entry.pinfonode->string;
    }
};


static const epicsPCIID anypci[] = {
    DEVPCI_DEVICE_VENDOR(DEVPCI_ANY_DEVICE, DEVPCI_ANY_VENDOR),
    DEVPCI_END
};

priv *parseLink(dbCommon *prec, const DBEntry& ent, unsigned vsize, priv::ORD ord)
{
    std::auto_ptr<priv> pvt(new priv);
    pvt->valsize = vsize;
    pvt->step = vsize;
    pvt->ord = ord;

    DBLINK *link = ent.getDevLink();
    if(link->type!=INST_IO)
        throw std::logic_error("No INST_IO");

    // auto read on initialization for output records
    pvt->initread = strcmp(ent.pentry()->pflddes->name, "OUT")==0;

    std::string linkstr(link->value.instio.string);

    size_t sep = linkstr.find_first_of(" \t");
    pvt->pciname = linkstr.substr(0, sep);

    const epicsPCIDevice *pdev = NULL;
    if(pvt->pciname!="test") {
        if(devPCIFindSpec(anypci, pvt->pciname.c_str(), &pdev, 0))
            throw std::runtime_error(SB()<<prec->name<<" Invalid PCI device "<<pvt->pciname);
    }

    sep = linkstr.find_first_not_of(" \t", sep);

    if(prec->tpro>1) {
        std::cerr<<prec->name<<" linkstr='"<<linkstr.substr(sep)<<"'\n";
    }

    // parse out "key=value ..." options
    while(sep<linkstr.size()) {
        size_t send = linkstr.find_first_of(" \t", sep),
               seq  = linkstr.find_first_of('=', sep);

        if(seq>=send)
            throw std::runtime_error(SB()<<"Expected '=' in '"<<linkstr.substr(0, send)<<"'");

        std::string optname(linkstr.substr(sep,seq-sep)),
                    optval (linkstr.substr(seq+1,send-seq-1));

        if(prec->tpro>1) {
            std::cerr<<prec->name<<" opt '"<<optname<<"'='"<<optval<<"'\n";
        }

        if(optname=="bar") {
            pvt->bar = parseU32(optval);
        } else if(optname=="offset") {
            pvt->offset = parseU32(optval);
        } else if(optname=="step") {
            pvt->step = parseU32(optval);
        } else if(optname=="mask") {
            pvt->vmask = parseU32(optval);
        } else if(optname=="shift") {
            pvt->vshift = parseU32(optval);
        } else if(optname=="initread") {
            pvt->initread = parseU32(optval)!=0;
        } else {
            throw std::runtime_error(SB()<<"Unknown option '"<<optname<<"'");
        }

        sep = linkstr.find_first_not_of(" \t", send);
    }

    if(prec->tpro>1) {
        std::cerr<<prec->name<<" : bar="<<pvt->bar
                 <<" offset="<<std::hex<<pvt->offset
                 <<" step="<<pvt->step
                 <<" mask="<<std::hex<<pvt->vmask
                 <<" shift="<<pvt->vshift
                 <<" size="<<pvt->valsize
                 <<" ord="<<(int)pvt->ord
                 <<"\n";
    }

    if(pdev) {
        if(devPCIToLocalAddr(pdev, pvt->bar, &pvt->base, 0))
            throw std::runtime_error(SB()<<prec->name<<" Failed to map bar "<<pvt->bar);
        if(devPCIBarLen(pdev, pvt->bar, &pvt->barsize))
            throw std::runtime_error(SB()<<prec->name<<" Failed to find size of bar "<<pvt->bar);
    } else {
        // testing mode
        pvt->base = exploreTestBase;
        pvt->barsize = exploreTestSize;
    }

    if(pvt->offset>=pvt->barsize || pvt->offset+pvt->valsize>pvt->barsize)
        throw std::runtime_error(SB()<<prec->name<<" offset "<<pvt->offset<<" out of range");

    return pvt.release();
}

template<int SIZE, priv::ORD ord>
long explore_init_record(dbCommon *prec)
{
    try {
        DBEntry ent(prec);
        std::auto_ptr<priv> pvt(parseLink(prec, ent, SIZE, ord));

        prec->dpvt = pvt.release();
        return 0;
    } catch(std::exception& e) {
        std::cerr<<prec->name<<" Error in init_record "<<e.what()<<"\n";
        return EINVAL;
    }
}

#define TRY if(!prec->dpvt) return 0; priv *pvt = (priv*)prec->dpvt; (void)pvt; try
#define CATCH() catch(std::exception& e) { std::cerr<<prec->name<<" Error : "<<e.what()<<"\n"; (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM); return 0; }

// integer to/from VAL

template<typename REC>
long explore_read_int_val(REC *prec)
{
    TRY {
        Guard G(pvt->lock);
        prec->val = pvt->read<epicsUInt32>();
        if(prec->tpro>1) {
            errlogPrintf("%s: read %08x -> VAL=%08x\n", prec->name, (unsigned)pvt->offset, (unsigned)prec->val);
        }
        return 0;
    } CATCH()
}

template<typename REC>
long explore_write_int_val(REC *prec)
{
    TRY {
        Guard G(pvt->lock);
        if(prec->tpro>1) {
            errlogPrintf("%s: write %08x <- VAL=%08x\n", prec->name, (unsigned)pvt->offset, (unsigned)prec->val);
        }
        pvt->write(prec->val);
        return 0;
    } CATCH()
}

template<typename REC, int SIZE, priv::ORD ord>
long explore_init_record_int_val(REC *prec)
{
    long ret = explore_init_record<SIZE,ord>((dbCommon*)prec);
    priv *pvt = (priv*)prec->dpvt;
    if(ret==0 && pvt->initread)
        ret = explore_read_int_val(prec);
    return ret;
}

// integer to/from RVAL

template<typename REC>
long explore_read_int_rval(REC *prec)
{
    TRY {
        Guard G(pvt->lock);
        prec->rval = pvt->read<epicsUInt32>();
        if(prec->tpro>1) {
            errlogPrintf("%s: read %08x -> RVAL=%08x\n", prec->name, (unsigned)pvt->offset, (unsigned)prec->rval);
        }
        return 0;
    } CATCH()
}

template<typename REC>
long explore_write_int_rval(REC *prec)
{
    TRY {
        Guard G(pvt->lock);
        if(prec->tpro>1) {
            errlogPrintf("%s: write %08x <- VAL=%08x\n", prec->name, (unsigned)pvt->offset, (unsigned)prec->rval);
        }
        pvt->write(prec->rval);
        return 0;
    } CATCH()
}

template<typename REC, int SIZE, priv::ORD ord>
long explore_init_record_int_rval(REC *prec)
{
    long ret = explore_init_record<SIZE,ord>((dbCommon*)prec);
    priv *pvt = (priv*)prec->dpvt;
    if(ret==0 && pvt->initread)
        ret = explore_read_int_rval(prec);
    return ret;
}

// float to/from VAL

template<typename REC>
long explore_read_real_val(REC *prec)
{

    TRY {
        punny32 pun;

        epicsUInt32 ival;
        {
            Guard G(pvt->lock);
            ival = pun.ival = pvt->read<epicsUInt32>();
        }

        pun.fval += prec->roff;
        if(prec->aslo) pun.fval *= prec->aslo;
        pun.fval += prec->aoff;
        if(prec->eslo) pun.fval *= prec->eslo;
        pun.fval += prec->eoff;

        prec->val = pun.fval;

        if(prec->tpro>1) {
            errlogPrintf("%s: read %08x -> %08x -> VAL=%g\n", prec->name, (unsigned)pvt->offset, (unsigned)ival, prec->val);
        }

        return 2;
    } CATCH()
}

template<typename REC>
long explore_write_real_val(REC *prec)
{
    TRY {
        punny32 pun;
        pun.fval = prec->val;

        pun.fval -= prec->eoff;
        if(prec->eslo) pun.fval /= prec->eslo;
        pun.fval -= prec->aoff;
        if(prec->aslo) pun.fval /= prec->aslo;
        pun.fval -= prec->roff;

        if(prec->tpro>1) {
            errlogPrintf("%s: write %08x <- %08x <- VAL=%g\n", prec->name, (unsigned)pvt->offset, (unsigned)pun.ival, prec->val);
        }

        Guard G(pvt->lock);
        pvt->write(pun.ival);

        return 0;
    } CATCH()
}

template<typename REC, int SIZE, priv::ORD ord>
long explore_init_record_real_val(REC *prec)
{
    long ret = explore_init_record<SIZE,ord>((dbCommon*)prec);
    priv *pvt = (priv*)prec->dpvt;
    if(ret==0 && pvt->initread)
        ret = explore_read_real_val(prec);
    return ret;
}

// Read into Waveform

long explore_read_wf(waveformRecord *prec)
{
    TRY {
        Guard G(pvt->lock);
        switch(prec->ftvl) {
        case menuFtypeUCHAR : prec->nord = pvt->readArray((epicsUInt8*)  prec->bptr, prec->nelm); break;
        case menuFtypeUSHORT: prec->nord = pvt->readArray((epicsUInt16*) prec->bptr, prec->nelm); break;
        case menuFtypeULONG : prec->nord = pvt->readArray((epicsUInt32*) prec->bptr, prec->nelm); break;
        case menuFtypeFLOAT : prec->nord = pvt->readArray((epicsFloat32*)prec->bptr, prec->nelm); break;
        default:
            (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        }

        return 0;
    } CATCH()
}

long explore_write_wf(waveformRecord *prec)
{
    TRY {
        Guard G(pvt->lock);
        unsigned nwritten = -1;
        switch(prec->ftvl) {
        case menuFtypeUCHAR : nwritten = pvt->writeArray((epicsUInt8*)  prec->bptr, prec->nord);
        case menuFtypeUSHORT: nwritten = pvt->writeArray((epicsUInt16*) prec->bptr, prec->nord);
        case menuFtypeULONG : nwritten = pvt->writeArray((epicsUInt32*) prec->bptr, prec->nord);
        case menuFtypeFLOAT : nwritten = pvt->writeArray((epicsFloat32*)prec->bptr, prec->nord);
        default:
            (void)recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
        }
        if(nwritten!=prec->nord)
            (void)recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
        return 0;
    } CATCH()
}

template<int SIZE, priv::ORD ord>
long explore_init_record_wf(waveformRecord *prec)
{
    long ret = explore_init_record<SIZE,ord>((dbCommon*)prec);
    priv *pvt = (priv*)prec->dpvt;
    if(ret==0 && pvt->initread)
        ret = explore_read_wf(prec);
    return ret;
}


template<typename REC>
struct dset6 {
    long nsup;
    long (*report)(int);
    long (*init)(int);
    long (*init_record)(REC*);
    long (*get_ioint_info)(int dir, REC*, IOSCANPVT*);
    long (*readwrite)(REC*);
    long (*unused)();
};

} // namespace

#define SUP(NAME, REC, OP, DIR, SIZE, END) static dset6<REC##Record> NAME = \
  {6, NULL, NULL, &explore_init_record_##OP<REC##Record,SIZE,END>, NULL, &explore_##DIR##_##OP<REC##Record>, NULL}; \
    epicsExportAddress(dset, NAME)

SUP(devExploreLiReadU8,     longin, int_val, read, 1, priv::NAT);
SUP(devExploreLiReadU16NAT, longin, int_val, read, 2, priv::NAT);
SUP(devExploreLiReadU16LSB, longin, int_val, read, 2, priv::LE);
SUP(devExploreLiReadU16MSB, longin, int_val, read, 2, priv::BE);
SUP(devExploreLiReadU32NAT, longin, int_val, read, 4, priv::NAT);
SUP(devExploreLiReadU32LSB, longin, int_val, read, 4, priv::LE);
SUP(devExploreLiReadU32MSB, longin, int_val, read, 4, priv::BE);

SUP(devExploreLoWriteU8,     longout, int_val, write, 1, priv::NAT);
SUP(devExploreLoWriteU16NAT, longout, int_val, write, 2, priv::NAT);
SUP(devExploreLoWriteU16LSB, longout, int_val, write, 2, priv::LE);
SUP(devExploreLoWriteU16MSB, longout, int_val, write, 2, priv::BE);
SUP(devExploreLoWriteU32NAT, longout, int_val, write, 4, priv::NAT);
SUP(devExploreLoWriteU32LSB, longout, int_val, write, 4, priv::LE);
SUP(devExploreLoWriteU32MSB, longout, int_val, write, 4, priv::BE);

SUP(devExploreBiReadU8,     bi, int_rval, read, 1, priv::NAT);
SUP(devExploreBiReadU16NAT, bi, int_rval, read, 2, priv::NAT);
SUP(devExploreBiReadU16LSB, bi, int_rval, read, 2, priv::LE);
SUP(devExploreBiReadU16MSB, bi, int_rval, read, 2, priv::BE);
SUP(devExploreBiReadU32NAT, bi, int_rval, read, 4, priv::NAT);
SUP(devExploreBiReadU32LSB, bi, int_rval, read, 4, priv::LE);
SUP(devExploreBiReadU32MSB, bi, int_rval, read, 4, priv::BE);

SUP(devExploreBoWriteU8,     bo, int_rval, write, 1, priv::NAT);
SUP(devExploreBoWriteU16NAT, bo, int_rval, write, 2, priv::NAT);
SUP(devExploreBoWriteU16LSB, bo, int_rval, write, 2, priv::LE);
SUP(devExploreBoWriteU16MSB, bo, int_rval, write, 2, priv::BE);
SUP(devExploreBoWriteU32NAT, bo, int_rval, write, 4, priv::NAT);
SUP(devExploreBoWriteU32LSB, bo, int_rval, write, 4, priv::LE);
SUP(devExploreBoWriteU32MSB, bo, int_rval, write, 4, priv::BE);

SUP(devExploreMbbiReadU8,     mbbi, int_rval, read, 1, priv::NAT);
SUP(devExploreMbbiReadU16NAT, mbbi, int_rval, read, 2, priv::NAT);
SUP(devExploreMbbiReadU16LSB, mbbi, int_rval, read, 2, priv::LE);
SUP(devExploreMbbiReadU16MSB, mbbi, int_rval, read, 2, priv::BE);
SUP(devExploreMbbiReadU32NAT, mbbi, int_rval, read, 4, priv::NAT);
SUP(devExploreMbbiReadU32LSB, mbbi, int_rval, read, 4, priv::LE);
SUP(devExploreMbbiReadU32MSB, mbbi, int_rval, read, 4, priv::BE);

SUP(devExploreMbboWriteU8,     mbbo, int_rval, write, 1, priv::NAT);
SUP(devExploreMbboWriteU16NAT, mbbo, int_rval, write, 2, priv::NAT);
SUP(devExploreMbboWriteU16LSB, mbbo, int_rval, write, 2, priv::LE);
SUP(devExploreMbboWriteU16MSB, mbbo, int_rval, write, 2, priv::BE);
SUP(devExploreMbboWriteU32NAT, mbbo, int_rval, write, 4, priv::NAT);
SUP(devExploreMbboWriteU32LSB, mbbo, int_rval, write, 4, priv::LE);
SUP(devExploreMbboWriteU32MSB, mbbo, int_rval, write, 4, priv::BE);

SUP(devExploreMbbiDirectReadU8,     mbbiDirect, int_rval, read, 1, priv::NAT);
SUP(devExploreMbbiDirectReadU16NAT, mbbiDirect, int_rval, read, 2, priv::NAT);
SUP(devExploreMbbiDirectReadU16LSB, mbbiDirect, int_rval, read, 2, priv::LE);
SUP(devExploreMbbiDirectReadU16MSB, mbbiDirect, int_rval, read, 2, priv::BE);
SUP(devExploreMbbiDirectReadU32NAT, mbbiDirect, int_rval, read, 4, priv::NAT);
SUP(devExploreMbbiDirectReadU32LSB, mbbiDirect, int_rval, read, 4, priv::LE);
SUP(devExploreMbbiDirectReadU32MSB, mbbiDirect, int_rval, read, 4, priv::BE);

SUP(devExploreMbboDirectWriteU8,     mbboDirect, int_rval, write, 1, priv::NAT);
SUP(devExploreMbboDirectWriteU16NAT, mbboDirect, int_rval, write, 2, priv::NAT);
SUP(devExploreMbboDirectWriteU16LSB, mbboDirect, int_rval, write, 2, priv::LE);
SUP(devExploreMbboDirectWriteU16MSB, mbboDirect, int_rval, write, 2, priv::BE);
SUP(devExploreMbboDirectWriteU32NAT, mbboDirect, int_rval, write, 4, priv::NAT);
SUP(devExploreMbboDirectWriteU32LSB, mbboDirect, int_rval, write, 4, priv::LE);
SUP(devExploreMbboDirectWriteU32MSB, mbboDirect, int_rval, write, 4, priv::BE);

SUP(devExploreAiReadU8,     ai, int_rval, read, 1, priv::NAT);
SUP(devExploreAiReadU16NAT, ai, int_rval, read, 2, priv::NAT);
SUP(devExploreAiReadU16LSB, ai, int_rval, read, 2, priv::LE);
SUP(devExploreAiReadU16MSB, ai, int_rval, read, 2, priv::BE);
SUP(devExploreAiReadU32NAT, ai, int_rval, read, 4, priv::NAT);
SUP(devExploreAiReadU32LSB, ai, int_rval, read, 4, priv::LE);
SUP(devExploreAiReadU32MSB, ai, int_rval, read, 4, priv::BE);

SUP(devExploreAoWriteU8,     ao, int_rval, write, 1, priv::NAT);
SUP(devExploreAoWriteU16NAT, ao, int_rval, write, 2, priv::NAT);
SUP(devExploreAoWriteU16LSB, ao, int_rval, write, 2, priv::LE);
SUP(devExploreAoWriteU16MSB, ao, int_rval, write, 2, priv::BE);
SUP(devExploreAoWriteU32NAT, ao, int_rval, write, 4, priv::NAT);
SUP(devExploreAoWriteU32LSB, ao, int_rval, write, 4, priv::LE);
SUP(devExploreAoWriteU32MSB, ao, int_rval, write, 4, priv::BE);

SUP(devExploreAiReadF32LSB, ai, real_val, read, 4, priv::LE);
SUP(devExploreAiReadF32MSB, ai, real_val, read, 4, priv::BE);

SUP(devExploreAoWriteF32LSB, ao, real_val, write, 4, priv::LE);
SUP(devExploreAoWriteF32MSB, ao, real_val, write, 4, priv::BE);

#undef SUP
#define SUP(NAME, DIR, SIZE, END) static dset6<waveformRecord> NAME = \
  {6, NULL, NULL, &explore_init_record_wf<SIZE,END>, NULL, &explore_##DIR##_wf, NULL}; \
    epicsExportAddress(dset, NAME)

SUP(devExploreWfReadU8,     read, 1, priv::NAT);
SUP(devExploreWfReadU16NAT, read, 2, priv::NAT);
SUP(devExploreWfReadU16LSB, read, 2, priv::LE);
SUP(devExploreWfReadU16MSB, read, 2, priv::BE);
SUP(devExploreWfReadU32NAT, read, 4, priv::NAT);
SUP(devExploreWfReadU32LSB, read, 4, priv::LE);
SUP(devExploreWfReadU32MSB, read, 4, priv::BE);

SUP(devExploreWfWriteU8,     write, 1, priv::NAT);
SUP(devExploreWfWriteU16NAT, write, 2, priv::NAT);
SUP(devExploreWfWriteU16LSB, write, 2, priv::LE);
SUP(devExploreWfWriteU16MSB, write, 2, priv::BE);
SUP(devExploreWfWriteU32NAT, write, 4, priv::NAT);
SUP(devExploreWfWriteU32LSB, write, 4, priv::LE);
SUP(devExploreWfWriteU32MSB, write, 4, priv::BE);
