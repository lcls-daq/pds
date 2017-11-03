#ifndef Pds_PvDaq_ControlsCameraServer_hh
#define Pds_PvDaq_ControlsCameraServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <vector>

namespace Pds {
  namespace PvDaq {
    class ConfigMonitor;
    class ConfigServer;
    class ImageServer;
    class ControlsCameraServer : public Pds::PvDaq::Server,
                                 public Pds_Epics::PVMonitorCb {
    public:
      enum Flags { SCALEPV = 0, UNIXCAM = 1 };
      ControlsCameraServer(const char*, const DetInfo&, const unsigned, const unsigned);
      ~ControlsCameraServer();
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
      ImageServer*                _image;
      ConfigServer*               _nrows;
      ConfigServer*               _ncols;
      ConfigServer*               _nbits;
      ConfigServer*               _gain;
      ConfigServer*               _model;
      ConfigServer*               _manufacturer;
      ConfigServer*               _exposure;
      ConfigServer*               _xscale;
      ConfigServer*               _yscale;
      ConfigServer*               _xorg;
      ConfigServer*               _xlen;
      ConfigServer*               _yorg;
      ConfigServer*               _ylen;
      std::vector<ConfigServer*>  _config_pvs;
      uint32_t                    _width;
      uint32_t                    _height;
      uint32_t                    _depth;
      uint32_t                    _offset;
      uint32_t                    _roi_x_org;
      uint32_t                    _roi_x_len;
      uint32_t                    _roi_x_end;
      uint32_t                    _roi_y_org;
      uint32_t                    _roi_y_len;
      uint32_t                    _roi_y_end;
      double                      _exposure_val;
      double                      _gain_val;
      double                      _xscale_val;
      double                      _yscale_val;
      char*                       _manufacturer_str;
      char*                       _model_str;
      bool                        _enabled;
      bool                        _configured;
      bool                        _scale;
      bool                        _unixcam;
      const unsigned              _max_evt_sz;
      size_t                      _frame_sz;
      ConfigMonitor*              _configMonitor;
      unsigned                    _nprint;
      ClockTime                   _last;
      unsigned                    _wrp;
      std::vector<char*>          _pool;
    };
  };
};

#endif
