#ifndef Pds_Zyla_ConfigCache_hh
#define Pds_Zyla_ConfigCache_hh

#include "andor3/include/atcore.h"

#include "pds/config/CfgCache.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  namespace Zyla {
    class Driver;
    class ConfigCache : public CfgCache {
    public:
      enum { MaxAtMsgLength=256, MaxErrMsgLength=512 };
      static ConfigCache* create(const Src& xtc, Driver& driver);
      ConfigCache(const Src& xtc, Driver& driver);
      virtual ~ConfigCache();
    public:
      bool configure(bool apply=true);
      const char* get_error() const;
      void clear_error();
    private:
      virtual bool _configure(bool apply) = 0;
      int _size (void*) const;
    protected:
      char*   _error;
      AT_WC*  _wc_buffer;
      Driver& _driver;
    };
  }
}

#endif

