#ifndef Pds_Epix10ka2m_ConfigCache_hh
#define Pds_Epix10ka2m_ConfigCache_hh

#include "pds/config/CfgCache.hh"
#include <vector>

namespace Pds {
  namespace Epix10ka2m {
    class Server;
    class ConfigCache : public CfgCache {
    public:
      ConfigCache(const Src& src);
    public:
      int  configure(const std::vector<Pds::Epix10ka2m::Server*>&);
      int  reformat (const Xtc* in, Xtc* out) const;
      void printCurrent();
    private:
      int  _size(void*) const;
    };
  };
};

#endif
