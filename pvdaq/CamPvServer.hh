#ifndef Pds_PvDaq_CamPvServer_hh
#define Pds_PvDaq_CamPvServer_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace PvDaq {
    class ImageServer : public Pds_Epics::EpicsCA {
      public:
        ImageServer(const char*, Pds_Epics::PVMonitorCb*, const int);
        ~ImageServer();
      public:
        const char* name() const;
        size_t elem_size() const;
        int  fetch      (void* copyTo, size_t len);
        void update     ();
      private:
        char* _name;
    };
  }
}

#endif
