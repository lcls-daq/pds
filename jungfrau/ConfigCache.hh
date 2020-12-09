#ifndef Pds_Jungfrau_ConfigCache_hh
#define Pds_Jungfrau_ConfigCache_hh

#include "pds/config/CfgCache.hh"
#include "pds/service/GenericPool.hh"
#include <vector>

namespace Pds {
  class Datagram;
  class Task;
  namespace Jungfrau {
    class Detector;
    class DetIdLookup;
    class Manager;
    class ConfigCache : public CfgCache {
    public:
      ConfigCache(const Src& dbsrc,
                  const Src& xtcsrc,
                  Detector& detector,
                  DetIdLookup& lookup,
                  Manager& mgr);
      virtual ~ConfigCache();
    public:
      bool configure(bool apply=true);
    private:
      int   _size (void*) const;
    private:
      GenericPool    _occPool;
      Detector&      _detector;
      DetIdLookup&   _lookup;
      Manager&       _mgr;
    };
  };
};

#endif

