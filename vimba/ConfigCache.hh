#ifndef Pds_Vimba_ConfigCache_hh
#define Pds_Vimba_ConfigCache_hh

#include "pds/config/CfgCache.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  namespace Vimba {
    class Camera;
    class ConfigCache : public CfgCache {
    public:
      enum { MaxAtMsgLength=256, MaxErrMsgLength=512 };
      static ConfigCache* create(const Src& xtc, Camera& cam);
      ConfigCache(const Src& xtc, Camera& cam);
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
      Camera& _cam;
    };
  }
}

#endif

