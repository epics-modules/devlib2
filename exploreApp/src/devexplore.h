#ifndef DEVEXPLORE_H
#define DEVEXPLORE_H

#ifdef __cplusplus

#include <map>
#include <string>
#include <sstream>
#include <stdexcept>

#include <epicsMutex.h>
#include <epicsGuard.h>
#include <dbStaticLib.h>
#include <dbAccess.h>

#include <shareLib.h>

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

typedef std::map<std::string, std::string> strmap_t;

epicsShareExtern
void parseToMap(const std::string& inp, strmap_t& ret);

epicsUInt32 parseU32(const std::string& s);

class DBEntry {
    DBENTRY entry;
public:
    DBENTRY *pentry() const { return const_cast<DBENTRY*>(&entry); }
    explicit DBEntry(dbCommon *prec) {
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


#endif /* __cplusplus */

#endif // DEVEXPLORE_H
