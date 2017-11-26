/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */

#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>

#include <dbAccess.h>
#include <dbBase.h>
#include <dbChannel.h>
#include <epicsMMIO.h>

#include <dbUnitTest.h>
#include <testMain.h>

#include <shareLib.h>

epicsShareExtern
volatile void * const exploreTestBase;
epicsShareExtern
const epicsUInt32 exploreTestSize;

extern "C"
int testexplore_registerRecordDeviceDriver(dbBase *pbase);

namespace {

struct Channel {
    dbChannel *chan;
    explicit Channel(const char *name)
        :chan(dbChannelCreate(name))
    {
        if(!chan)
            testAbort("No channel %s", name);
        if(dbChannelOpen(chan))
            testAbort("Can't open channel %s", name);
    }
    ~Channel() {
        dbChannelDelete(chan);
    }
    operator dbChannel*() const { return chan; }
    void get_int32(std::vector<epicsUInt32>& val) {
        val.resize(dbChannelFinalElements(chan));
        long nReq = (long)val.size();
        if(dbChannelGetField(chan, DBF_ULONG, &val[0], NULL, &nReq, NULL))
            testAbort("get %s fails", dbChannelName(chan));
        val.resize(nReq);
    }
    void put_int32(const std::vector<epicsUInt32>& val) {
        if(dbChannelPutField(chan, DBF_ULONG, &val[0], val.size()))
            testAbort("get %s fails", dbChannelName(chan));
    }
};

#define testEqual(A,B,msg) testOk((A)==(B), #A " (0x%x) == " #B " (0x%x) %s", (unsigned)(A), (unsigned)(B), msg)

template<typename V>
void testRead(const char *recname, unsigned offset, unsigned size, epicsUInt32 store, short dbf, V expect)
{
    if(offset>=exploreTestSize || offset+size>exploreTestSize)
        testAbort("Invalid offset %x", offset);
    {
        volatile char * A= offset+(volatile char*)exploreTestBase;
        epicsUInt32 T=store;
        switch(size) {
        case 1:
            iowrite8(A, T);
            break;
        case 2:
            be_iowrite16(A, T);
            break;
        case 4:
            be_iowrite32(A, T);
            break;
        default:
            testAbort("testRead size=%u", size);
        }
    }

    std::string proc(recname);
    proc+=".PROC";
    testdbPutFieldOk(proc.c_str(), DBF_ULONG, 1);
    testdbGetFieldEqual(recname, dbf, expect);
}

void writeVal(unsigned offset, epicsUInt32 val)
{
    volatile char * A= offset+(volatile char*)exploreTestBase;
    be_iowrite32(A, val);
}

void testVal(unsigned offset, epicsUInt32 expect, const char *msg="")
{
    volatile char * A= offset+(volatile char*)exploreTestBase;
    epicsUInt32 actual = be_ioread32(A);
    testEqual(actual, expect, msg);
}

template<typename V>
void testWrite(const char *recname, unsigned offset, unsigned size, epicsUInt32 expect, short dbf, V store)
{
    if(offset>=exploreTestSize || offset+size>exploreTestSize)
        testAbort("Invalid offset %x", offset);

    testdbPutFieldOk(recname, dbf, store);
    epicsUInt32 actual = 0;
    volatile char * A= offset+(volatile char*)exploreTestBase;

    switch(size) {
    case 1:
        actual = ioread8(A);
        break;
    case 2:
        actual = be_ioread16(A);
        break;
    case 4:
        actual = be_ioread32(A);
        break;
    default:
        testAbort("testWrite size=%u", size);
    }
    testOk(actual==expect, "testWrite %s %08x == %08x", recname, (unsigned)actual, (unsigned)expect);
}

void testInitial()
{
    testDiag("Read initial values");
    /* initial "register" values [0xf0, 0xf1, 0xf2, 0xf3, ...] */
    testdbGetFieldEqual("longin8",   DBF_ULONG, 0xf1);
    testdbGetFieldEqual("longin8_1", DBF_ULONG, 0xf1&0x8f);
    testdbGetFieldEqual("longin8_2", DBF_ULONG, (0xf2&0xf0)>>4);
    testdbGetFieldEqual("longin8_x", DBF_ULONG, 0); // initread=0

    testdbGetFieldEqual("longin16",  DBF_ULONG, 0xf2f3);
    testdbGetFieldEqual("longin16_1",DBF_ULONG, 0xf3f2);

    testdbGetFieldEqual("longin32",  DBF_ULONG, 0xf4f5f6f7);
    testdbGetFieldEqual("longin32_1",DBF_ULONG, 0xf7f6f5f4);

    testdbGetFieldEqual("longout8",  DBF_ULONG, 0xf0);
    testdbGetFieldEqual("longout8_1",DBF_ULONG, 0xf);

    testdbGetFieldEqual("longout16",  DBF_ULONG, 0xf2f3);
    testdbGetFieldEqual("longout16_1",DBF_ULONG, 0xf3f2);
    testdbGetFieldEqual("longout16_2",DBF_ULONG,  0x2f);

    testdbGetFieldEqual("longout32",  DBF_ULONG, 0xf4f5f6f7);
    testdbGetFieldEqual("longout32_1",DBF_ULONG, 0xf7f6f5f4);

    union {
        epicsUInt32 ival;
        epicsFloat32 fval;
    } pun;
    pun.ival = 0xf4f5f6f7;
    testDiag("expected float val 0x%08x -> %g", (unsigned)pun.ival, pun.fval);
    testdbGetFieldEqual("floatin32",  DBF_FLOAT, pun.fval);
    testdbGetFieldEqual("floatout32", DBF_FLOAT, pun.fval);

    {
        std::vector<epicsUInt32> val;

        Channel chan("wfin32");
        testDiag("initial value of %s", dbChannelName(chan.chan));

        chan.get_int32(val);
        testOk1(val.size()==2);
        val.resize(2);
        testEqual(val[0], 0xf4f5f6f7, "");
        testEqual(val[1], 0xf8f9fafb, "");
    }
    {
        std::vector<epicsUInt32> val;

        Channel chan("wfin32_1");
        testDiag("initial value of %s", dbChannelName(chan.chan));

        chan.get_int32(val);
        testOk1(val.size()==2);
        val.resize(2);
        testEqual(val[0], 0xf4f5f6f7, "");
        testEqual(val[1], 0xf4f5f6f7, "");
    }
    {
        std::vector<epicsUInt32> val;

        Channel chan("wfout32");
        testDiag("initial value of %s", dbChannelName(chan.chan));

        chan.get_int32(val);
        testOk1(val.size()==2);
        val.resize(2);
        testEqual(val[0], 0xf4f5f6f7, "");
        testEqual(val[1], 0xf8f9fafb, "");
    }
    {
        std::vector<epicsUInt32> val;

        Channel chan("wfout32_1");
        testDiag("initial value of %s", dbChannelName(chan.chan));

        chan.get_int32(val);
        testOk1(val.size()==2);
        val.resize(2);
        testEqual(val[0], 0xf4f5f6f7, "");
        testEqual(val[1], 0xf4f5f6f7, "");
    }
}

void testScalarRead()
{
    testDiag("Read scalar values");

    testRead("longin8",   1, 1, 0x7c, DBF_ULONG, 0x7c);
    testRead("longin8_1", 1, 1, 0x7c, DBF_ULONG, 0x7c&0x8f);
    testRead("longin8_2", 2, 1, 0x7c, DBF_ULONG, 0x7);

    testRead("longin16",   2, 2, 0xabcd, DBF_ULONG, 0xabcd);
    testRead("longin16_1", 2, 2, 0xabcd, DBF_ULONG, 0xcdab);

    testRead("longin32",   4, 4, 0x01020304, DBF_ULONG, 0x01020304);
    testRead("longin32_1", 4, 4, 0x01020304, DBF_ULONG, 0x04030201);
}

void testScalarWrite()
{
    testDiag("Write scalar values");

    testWrite("longout8",   0, 1, 0xab, DBF_ULONG, 0xab);
    // RMW assumes previous success
    testWrite("longout8_1", 0, 1, 0x3b, DBF_ULONG, 3);

    testWrite("longout16",  2, 2, 0xabcd, DBF_ULONG, 0xabcd);
    testWrite("longout16_1",2, 2, 0xabcd, DBF_ULONG, 0xcdab);
    // RMW assumes previous success
    testWrite("longout16_2",2, 2, 0xa12d, DBF_ULONG, 0x12);

    testWrite("longout32",  4, 4, 0x01020304, DBF_ULONG, 0x01020304);
    testWrite("longout32_1",4, 4, 0x01020304, DBF_ULONG, 0x04030201);
}

void testFloatRW()
{
    testDiag("Test read/write of float32 values");

    union {
        epicsUInt32 ival;
        epicsFloat32 fval;
    } pun;
    pun.ival = 0x01040203;

    testRead("floatin32" , 4, 4, pun.ival, DBF_FLOAT, pun.fval);

    pun.ival = 0xdeadbeef;
    testWrite("floatout32", 4, 4, pun.ival, DBF_FLOAT, pun.fval);
}

void testWF()
{
    testDiag("read/write uint32 array");

    std::vector<epicsUInt32> val;
    Channel wfin32("wfin32"),
            wfin32_1("wfin32_1"),
            wfout32("wfout32"),
            wfout32_1("wfout32_1");

    writeVal(4, 0x12345678);
    writeVal(8, 0x9abcdef0);

    testDiag("read array step=4");
    testdbPutFieldOk("wfin32.PROC", DBF_LONG, 1);
    wfin32.get_int32(val);
    testEqual(val.size(), 2, "");
    val.resize(2);
    testEqual(val[0], 0x12345678, "");
    testEqual(val[1], 0x9abcdef0, "");

    testDiag("read array step=0");
    testdbPutFieldOk("wfin32_1.PROC", DBF_LONG, 1);
    wfin32_1.get_int32(val);
    testEqual(val.size(), 2, "");
    val.resize(2);
    testEqual(val[0], 0x12345678, "");
    testEqual(val[1], 0x12345678, "");

    testDiag("write array step=4");
    val.resize(2);
    val[0] = 0xdeadbeef;
    val[1] = 0x1badface;
    wfout32.put_int32(val);
    testVal(4, 0xdeadbeef);
    testVal(8, 0x1badface);
}

} // namespace

MAIN(testexplore)
{
    testPlan(71);

    {
        volatile char *base = (volatile char*)exploreTestBase;
        for(epicsUInt32 i=0; i<exploreTestSize; i++)
            base[i] = 0xf0 | i;
    }

    testdbPrepare();

    testdbReadDatabase("testexplore.dbd", NULL, NULL);

    testexplore_registerRecordDeviceDriver(pdbbase);

    testdbReadDatabase("testexplore.db", NULL, NULL);

    testIocInitOk();

    testInitial();
    testScalarRead();
    testScalarWrite();
    testFloatRW();
    testWF();

    testIocShutdownOk();

    testdbCleanup();

    return testDone();
}
