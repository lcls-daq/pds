#ifndef Pds_PvDaq_AcqirisServer_hh
#define Pds_PvDaq_AcqirisServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <vector>

namespace Pds {
  namespace PvDaq {
    class ConfigMonitor;
    class ConfigServer;
    class AcqirisPvServer;
    class AcqirisServer : public Pds::PvDaq::Server,
                          public Pds_Epics::PVMonitorCb {
    public:
      AcqirisServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      ~AcqirisServer();
    public:
      // Server interface
      int         fill( char*, const void* );
      Transition* fire(Transition*);
      InDatagram* fire(InDatagram*);
      // PvMonitorCb
      void        updated();
      void        config_updated();
    private:
      char*                       _pvchan;
      char*                       _pvcard;
      const DetInfo&              _info;
      AcqConfigType*              _cfg;
      AcqirisPvServer*            _waveformPv;
      ConfigServer*               _fullScalePv;
      ConfigServer*               _offsetPv;
      ConfigServer*               _couplingPv;
      ConfigServer*               _bandwidthPv;
      ConfigServer*               _sampIntervalPv;
      ConfigServer*               _delayTimePv;
      ConfigServer*               _nbrSamplesPv;
      ConfigServer*               _nbrSegmentsPv;
      ConfigServer*               _trigCouplingPv;
      ConfigServer*               _trigInputPv;
      ConfigServer*               _trigSlopePv;
      ConfigServer*               _trigLevelPv;
      ConfigServer*               _nbrConvPerChanPv;
      ConfigServer*               _nbrBanksPv;
      std::vector<ConfigServer*>  _config_pvs;
      char*                       _data_pvname;
      double                      _fullScale;
      double                      _offset;
      char*                       _coupling;
      char*                       _bandwidth;
      double                      _sampInterval;
      double                      _delayTime;
      uint32_t                    _nbrSamples;
      uint32_t                    _nbrSegments;
      char*                       _trigCoupling;
      char*                       _trigInput;
      char*                       _trigSlope;
      double                      _trigLevel;
      uint32_t                    _nbrConvPerChan;
      uint32_t                    _nbrBanks;
      bool                        _enabled;
      bool                        _configured;
      const unsigned              _max_evt_sz;
      size_t                      _waveform_sz;
      ConfigMonitor*              _configMonitor;
      unsigned                    _nprint;
      ClockTime                   _last;
      unsigned                    _wrp;
      std::vector<char*>          _pool;
      ca_client_context*          _context;
    };
  }
}

#endif
