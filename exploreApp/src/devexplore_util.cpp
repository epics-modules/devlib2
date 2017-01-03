
#include <stdexcept>

#include <errno.h>

#include <epicsVersion.h>
#include <epicsStdlib.h>

#include "devexplore.h"

void parseToMap(const std::string& inp, strmap_t& ret)
{
    ret.clear();

    size_t sep = inp.find_first_not_of(" \t");

    while(sep<inp.size()) {
        // expect "key=val" with no internal whitespace
        size_t send = inp.find_first_of(" \t", sep),
               seq  = inp.find_first_of('=', sep);

        if(seq>=send)
            throw std::runtime_error(SB()<<"Expected '=' in '"<<inp.substr(0, send)<<"'");

        std::string optname(inp.substr(sep,seq-sep)),
                    optval (inp.substr(seq+1,send-seq-1));

        ret[optname] = optval;

        sep = inp.find_first_not_of(" \t", send);
    }
}

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
#endif /* EPICS_REVISION<=14 */



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
