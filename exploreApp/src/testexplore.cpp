/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */

#include <iostream>
#include <string>

#include <dbAccess.h>
#include <dbBase.h>

#include <dbUnitTest.h>
#include <testMain.h>

extern
volatile void * const exploreTestBase;
extern
const epicsUInt32 exploreTestSize;

extern "C"
int testexplore_registerRecordDeviceDriver(dbBase *pbase);

namespace {

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
            *A++ = (T>>0)&0xff;
            break;
        case 2:
            *A++ = (T>>8)&0xff;
            *A++ = (T>>0)&0xff;
            break;
        case 4:
            *A++ = (T>>24)&0xff;
            *A++ = (T>>16)&0xff;
            *A++ = (T>>8)&0xff;
            *A++ = (T>>0)&0xff;
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
        actual |= (*A++)<<0;
        actual &= 0xff;
        break;
    case 2:
        actual |= (*A++)<<8;
        actual |= (*A++)<<0;
        actual &= 0xffff;
        break;
    case 4:
        actual |= (*A++)<<24;
        actual |= (*A++)<<16;
        actual |= (*A++)<<8;
        actual |= (*A++)<<0;
        actual &= 0xffffffff;
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

    testdbGetFieldEqual("longout32",  DBF_ULONG, 0xf4f5f6f7);
    testdbGetFieldEqual("longout32_1",DBF_ULONG, 0xf7f6f5f4);
}

void testScalarRead()
{
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
    testWrite("longout8",   0, 1, 0xab, DBF_ULONG, 0xab);
    // RMW assumes previous success
    testWrite("longout8_1", 0, 1, 0x3b, DBF_ULONG, 3);

    testWrite("longout16",  2, 2, 0xabcd, DBF_ULONG, 0xabcd);
    testWrite("longout16_1",2, 2, 0xabcd, DBF_ULONG, 0xcdab);

    testWrite("longout32",  4, 4, 0x01020304, DBF_ULONG, 0x01020304);
    testWrite("longout32_1",4, 4, 0x01020304, DBF_ULONG, 0x04030201);
}

} // namespace

MAIN(testexplore)
{
    testPlan(0);

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

    testIocShutdownOk();

    testdbCleanup();

    return testDone();
}
