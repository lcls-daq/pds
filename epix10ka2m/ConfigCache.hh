#ifndef Pds_Epix10ka2m_ConfigCache_hh
#define Pds_Epix10ka2m_ConfigCache_hh

#include "pds/config/PgpCfgCache.hh"
#include <vector>

namespace Pds {
  class Datagram;
  class Task;
  namespace Epix10ka2m {
    class Server;
    class ConfigRoutine;
    class ConfigCache : public PgpCfgCache {
    public:
      ConfigCache(const Src& src);
      virtual ~ConfigCache();
    public:
      int  configure(const std::vector<Pds::Epix10ka2m::Server*>&);
      void reformat (const Datagram& in, Datagram& out) const;
      void printCurrent();
      void record   (InDatagram*);
    private:
      int   _size (void*) const;
    private:
      Task*          _task   [4];
      ConfigRoutine* _routine[4];
      char*          _cache;
    };
  };
};

#endif
