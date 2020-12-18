#ifndef Pds_Zyla_iStarConfigCache_hh
#define Pds_Zyla_iStarConfigCache_hh

#include "pds/config/ZylaConfigType.hh"
#include "pds/zyla/ConfigCache.hh"

namespace Pds {
  namespace Zyla {
    class iStarConfigCache : public ConfigCache {
    public:
      iStarConfigCache(const Src& xtc, Driver& driver);
      virtual ~iStarConfigCache();
      virtual bool configure(bool apply=true);
    private:
      const iStarConfigType* _cfg;
    };
  }
}

#endif
