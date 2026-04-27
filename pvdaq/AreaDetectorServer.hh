#ifndef Pds_PvDaq_AreaDetectorServer_hh
#define Pds_PvDaq_AreaDetectorServer_hh

#include "pds/pvdaq/Server.hh"
#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/epicstools/EpicsCA.hh"

#include <vector>

namespace Pds {
  namespace PvDaq {
    class ImageServer;
    class AreaDetectorServer : public Pds::PvDaq::Server,
                               public Pds_Epics::PVMonitorCb {
    public:
      enum Flags { SCALEPV = 0, UNIXCAM = 1 };
      AreaDetectorServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~AreaDetectorServer();
    public:
      // Server interface
      int         fill( char*, const void* );
      Transition* fire(Transition*);
      InDatagram* fire(InDatagram*);
      // PvMonitorCb
      void        updated();
      void        config_updated();
    protected:
      virtual bool configure() = 0;
      virtual void show_configuration() = 0;
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg) = 0;
      virtual char* alloc_data_type(Xtc& xtc) = 0;
      void create_pv_name(char* pvname, size_t len, const char* name, const char* prefix);
      void create_pv(const char* name, const char* prefix, bool is_enum, void* copyTo, size_t len, ConfigServer::Type type);
      static const unsigned ENUM_PV_LEN;
    protected:
      const char*                 _pvbase;
      const char*                 _pvimage;
      const DetInfo&              _info;
      ImageServer*                _image;
      std::vector<ConfigServer*>  _config_pvs;
      uint32_t                    _width;
      uint32_t                    _height;
      uint32_t                    _depth;
      uint32_t                    _bytes;
      uint32_t                    _offset;
      uint32_t                    _roi_x_org;
      uint32_t                    _roi_x_len;
      uint32_t                    _roi_x_end;
      uint32_t                    _roi_y_org;
      uint32_t                    _roi_y_len;
      uint32_t                    _roi_y_end;
      uint32_t                    _max_width;
      uint32_t                    _max_height;
      double                      _exposure;
      double                      _gain;
      double                      _xscale;
      double                      _yscale;
      char*                       _manufacturer;
      char*                       _model;
      char*                       _serial_num;
      char*                       _firmware_ver;
      char*                       _image_pvname;
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
      ca_client_context*          _context;
    };

    class EpicsCamServer : public AreaDetectorServer {
    public:
      EpicsCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~EpicsCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual char* alloc_data_type(Xtc& xtc);
    };
  }
}

#endif
