#ifndef Pds_EvrCfgCache_hh
#define Pds_EvrCfgCache_hh

#include "pds/config/CfgCache.hh"

namespace Pds {
  class EvrCfgCache : public CfgCache {
  public:
    EvrCfgCache();
    virtual ~EvrCfgCache();
    void      init (const Allocation&);
    unsigned  group() const;
    unsigned  code () const;
  private:
    int _size (void*) const;
  private:
    unsigned _group;
  };
}

#endif
