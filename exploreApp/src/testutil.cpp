
#include <exception>

#include <epicsUnitTest.h>
#include <testMain.h>

#include "devexplore.h"

namespace {

void testParseLink()
{
    strmap_t ret;

    testDiag("Parse empty string");
    parseToMap("", ret);
    testOk1(ret.empty());

    testDiag("Parse whitespace string");
    parseToMap("  \t  ", ret);
    testOk1(ret.empty());

    testDiag("Parse single key value");
    parseToMap("  K=V  ", ret);
    testOk1(ret.size()==1);
    if(ret.find("K")!=ret.end())
        testOk(ret["K"]=="V", "ret[\"K\"] = %s", ret["K"].c_str());
    else
        testFail("Missing key K");

    testDiag("Parse two key value");
    parseToMap("  K=V  XYZ=ABC  ", ret);
    testOk1(ret.size()==2);
    if(ret.find("K")!=ret.end())
        testOk(ret["K"]=="V", "ret[\"K\"] = %s", ret["K"].c_str());
    else
        testFail("Missing key K");
    if(ret.find("XYZ")!=ret.end())
        testOk(ret["XYZ"]=="ABC", "ret[\"XYZ\"] = %s", ret["XYZ"].c_str());
    else
        testFail("Missing key XYZ");
}

} // namespace

MAIN(testutil)
{
    testPlan(0);
    try {
        testParseLink();
    }catch(std::exception& e) {
        testAbort("Unexpected c++ exception: %s", e.what());
    }
    return testDone();
}
