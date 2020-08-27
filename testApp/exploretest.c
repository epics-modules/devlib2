
#include <osiSock.h>

#include <dbAccess.h>
#include <errlog.h>
#include <dbUnitTest.h>
#include <testMain.h>

#include <devLibPCI.h>
#include <devLibPCIImpl.h>

static
volatile
epicsUInt32 bar0[4];

typedef union {
    epicsUInt32 u32;
    epicsFloat32 f32;
} pun32;

static
float u2f(epicsUInt32 i) {
    pun32 pun;
    pun.u32 = i;
    return pun.f32;
}

static
epicsUInt32 f2u(float f) {
    pun32 pun;
    pun.f32 = f;
    return pun.u32;
}

/* we simulate a bus with single PCI device with a single BAR
 *
 * 0:1.0 device=0x1010 vendor=0x2020 subdevice=0x3030 subvendor=0x4040
 *       class=0x0e80 revision=1
 */
static
const epicsPCIDevice dev010 = {
    DEVPCI_SUBDEVICE_SUBVENDOR_CLASS(0x1010, 0x2020, 0x3030, 0x4040, 1, 0x0e80),
    0, 1, 0,
    "testslot",
    {
        {0,0,0},
        {0,0,0},
        {0,0,0},
        {0,0,0},
        {0,0,0},
        {0,0,0},
    },
    0,
    0,
    "pcisim",
};

static
int sim_init(void)
{
    size_t i;
    volatile epicsUInt8* bar = (volatile epicsUInt8*)bar0;

    for(i=0; i<sizeof(bar0); i++)
        bar[i] = i;

    bar0[0] = htonl(f2u(1.5));
    return 0;
}

static
int sim_find(const epicsPCIID *ids, devPCISearchFn searchfn, void *arg, unsigned int o)
{
    unsigned i;
    const epicsPCIID *search;

    for(search=ids, i=0; search->device!=DEVPCI_LAST_DEVICE; search++, i++){
        int err;

        if(!devLibPCIMatch(search, &dev010.id)) {
            testDiag("Device mis-match");
            continue;
        }

        err=searchfn(arg,&dev010);
        if(err==0) /* Continue search */
            continue;
        else if(err==1) /* Abort search OK */
            return 0;
        else /* Abort search Err */
            return err;
    }

    return 0;
}

static
int sim_map(const epicsPCIDevice* dev,unsigned int bar,volatile void **addr,unsigned int o)
{
    if(dev==&dev010 && bar==0) {
        *addr = (volatile void*)bar0;
        return 0;
    }
    return S_dev_addrMapFail;
}

static
int sim_len(const epicsPCIDevice* dev,unsigned int bar,epicsUInt32 *len)
{
    if(dev==&dev010 && bar==0) {
        *len = sizeof(bar0);
        return 0;
    }
    return S_dev_badCard;
}

static
devLibPCI pcisim = {
    "pcisim",
    &sim_init,
    NULL,
    &sim_find,
    &sim_map,
    &sim_len,
    NULL,
    NULL,
    NULL,
    NULL,
};

void exploretest_registerRecordDeviceDriver(struct dbBase *);

MAIN(exploretest)
{
    testPlan(16);

    /* override action of registrar */
    testOk1(devLibPCIRegisterDriver(&pcisim)==0);

    testdbPrepare();

    testdbReadDatabase("exploretest.dbd", NULL, NULL);
    exploretest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("exploretest.db", NULL, "");

    testOk1(devLibPCIUse("pcisim")==0);

    eltc(0);
    testIocInitOk();
    eltc(1);

    testDiag("Test reads");

    testdbGetFieldEqual("in_u8", DBF_ULONG, 0x04);
    testdbGetFieldEqual("in_u16", DBF_ULONG, 0x0405);
    testdbGetFieldEqual("in_u32", DBF_ULONG, 0x04050607);
    testdbGetFieldEqual("in_f32", DBF_DOUBLE, 1.5);

    testdbGetFieldEqual("in_vec_u16.NORD", DBF_ULONG, 6);
    static const epicsUInt16 expectarr16[] = {0x0405, 0x0607, 0x0809, 0x0a0b, 0x0c0d, 0x0e0f};
    testdbGetArrFieldEqual("in_vec_u16", DBF_USHORT, 32, NELEMENTS(expectarr16), expectarr16);

    testDiag("Test writes");

    bar0[1] = 0;
    testdbPutFieldOk("out_u8", DBF_ULONG, 0x0fffff42);
    testOk(ntohl(bar0[1])==0x42000000, "bar0[1] 0x42000000 == %08x", (unsigned)ntohl(bar0[1]));

    bar0[1] = 0;
    testdbPutFieldOk("out_u16", DBF_ULONG, 0x0fff5242);
    testOk(ntohl(bar0[1])==0x52420000, "bar0[1] 0x52420000 == %08x", (unsigned)ntohl(bar0[1]));

    bar0[1] = 0;
    testdbPutFieldOk("out_u32", DBF_ULONG, 0x80706050);
    testOk(ntohl(bar0[1])==0x80706050, "bar0[1] 0x80706050 == %08x", (unsigned)ntohl(bar0[1]));

    bar0[0] = 0;
    testdbPutFieldOk("out_f32", DBF_DOUBLE, 2.25);
    testOk(u2f(ntohl(bar0[0]))==2.25, "bar0[1] 2.25 == %f", u2f(ntohl(bar0[0])));

    testIocShutdownOk();

    testdbCleanup();

    return testDone();
}
