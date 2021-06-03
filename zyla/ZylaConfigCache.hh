#ifndef Pds_Zyla_ZylaConfigCache_hh
#define Pds_Zyla_ZylaConfigCache_hh

#include "pds/config/ZylaConfigType.hh"
#include "pds/zyla/ConfigCache.hh"

namespace Pds {
  namespace Zyla {
    class ZylaConfigCache : public ConfigCache {
    public:
      ZylaConfigCache(const Src& xtc, Driver& driver);
      virtual ~ZylaConfigCache();
    private:
      virtual bool _configure(bool apply);
    private:
      const ZylaConfigType* _cfg;
    };
  }
}

#endif
