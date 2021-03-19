#ifndef Pds_PvDaq_WaveformServer_hh
#define Pds_PvDaq_WaveformServer_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace PvDaq {
    class QuadAdcPvServer : public Pds_Epics::EpicsCA {
      public:
        QuadAdcPvServer(const char*, Pds_Epics::PVMonitorCb*, const unsigned, const unsigned);
        ~QuadAdcPvServer();
      public:
        const char* name() const;
        int  fetch      (void* copyTo, size_t len);
        void update     ();
      private:
        char* _name;
    };
  }
}

#endif
