#ifndef Pds_PvDaq_WaveformServer_hh
#define Pds_PvDaq_WaveformServer_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  namespace PvDaq {
    class AcqirisPvServer : public Pds_Epics::EpicsCA {
      public:
        AcqirisPvServer(const char*, Pds_Epics::PVMonitorCb*, const unsigned);
        ~AcqirisPvServer();
      public:
        const char* name() const;
        int  fetch      (void* copyTo, size_t len);
        void update     ();
      private:
        char* _name;
    };

    class QuadAdcPvServer : public Pds_Epics::EpicsCA {
      public:
        QuadAdcPvServer(const char*,
                        Pds_Epics::PVMonitorCb*,
                        const unsigned nchans,
                        const unsigned elems,
                        const double offset,
                        const double range,
                        const double scale,
                        const bool sparse,
                        const unsigned sparse_lo,
                        const unsigned sparse_hi);
        ~QuadAdcPvServer();
      public:
        const char* name() const;
        int  fetch      (void* copyTo, size_t len);
        void update     ();
      private:
        char*    _name;
        double   _offset;
        double   _range;
        double   _scale;
        bool     _sparse;
        unsigned _fill;
    };
  }
}

#endif
