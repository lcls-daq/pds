#ifndef Pds_PvDaq_QuadAdcServer_hh
#define Pds_PvDaq_QuadAdcServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <vector>

namespace Pds {
  namespace PvDaq {
    class ConfigMonitor;
    class ConfigServer;
    class QuadAdcPvServer;
    class QuadAdcServer : public Pds::PvDaq::Server,
                          public Pds_Epics::PVMonitorCb {
    public:
      QuadAdcServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      ~QuadAdcServer();
    public:
      // Server interface
      int         fill( char*, const void* );
      Transition* fire(Transition*);
      InDatagram* fire(InDatagram*);
      // PvMonitorCb
      void        updated();
      void        config_updated();
    private:
      const char*                 _pvbase;
      const char*                 _pvdata;
      const DetInfo&              _info;
      QuadAdcPvServer*            _waveformPv;
      ConfigServer*               _ichanPv;
      ConfigServer*               _trigEventPv;
      ConfigServer*               _trigDelayPv;
      ConfigServer*               _lengthPv;
      ConfigServer*               _interleavePv;
      std::vector<ConfigServer*>  _config_pvs;
      char*                       _data_pvname;
      int32_t                     _inputChan;
      uint32_t                    _chanMask;
      uint32_t                    _evtCode;
      uint32_t                    _evtDelay;
      double                      _delayTime;
      uint32_t                    _nbrSamples;
      uint32_t                    _interleave;
      double                      _sampleRate;   
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
