#ifndef Pds_Epix10ka2m_ConfigCache_hh
#define Pds_Epix10ka2m_ConfigCache_hh

#include "pds/config/CfgCache.hh"
#include <vector>

namespace Pds {
  class Task;
  namespace Epix10ka2m {
    class Server;
    class ConfigRoutine;
    class ConfigCache : public CfgCache {
    public:
      ConfigCache(const Src& src);
      ~ConfigCache();
    public:
      int  configure(const std::vector<Pds::Epix10ka2m::Server*>&);
      int  reformat (const Xtc* in, Xtc* out) const;
      void printCurrent();
    private:
      int   _size (void*) const;
    private:
      Task*          _task   [4];
      ConfigRoutine* _routine[4];
    };
  };
};

#endif
