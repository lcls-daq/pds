#ifndef Pds_Vimba_AlviumConfigCache_hh
#define Pds_Vimba_AlviumConfigCache_hh

#include "pds/config/VimbaConfigType.hh"
#include "pds/vimba/ConfigCache.hh"

namespace Pds {
  namespace Vimba {
    class AlviumConfigCache : public ConfigCache {
    public:
      AlviumConfigCache(const Src& xtc, Camera& cam);
      virtual ~AlviumConfigCache();
    private:
      virtual bool _configure(bool apply);
    };
  }
}

#endif
