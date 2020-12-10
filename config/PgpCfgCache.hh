#ifndef Pds_PgpCfgCache_hh
#define Pds_PgpCfgCache_hh

#include "pds/config/CfgCache.hh"

namespace Pds {
  class EvrCfgCache;

  class PgpCfgCache : public CfgCache {
  public:
    PgpCfgCache(const Src&    src,
                const TypeId& id,
                int           size);
    PgpCfgCache(const Src&    dbsrc,
                const Src&    xtcsrc,
                const TypeId& id,
                int           size);
    virtual ~PgpCfgCache();
    void      init (const Allocation&);
    int       fetch(const Transition*);
    unsigned  group() const;
    unsigned  code () const;
  private:
    EvrCfgCache* _evr;
  };
}

#endif
