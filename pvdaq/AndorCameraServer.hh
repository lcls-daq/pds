#ifndef Pds_PvDaq_AndorCameraServer_hh
#define Pds_PvDaq_AndorCameraServer_hh

#include "pds/pvdaq/AreaDetectorServer.hh"
#include "pds/config/ZylaConfigType.hh"

namespace Pds {
  namespace PvDaq {
    template <typename Cfg>
    class AndorCamServer : public AreaDetectorServer {
    public:
      AndorCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~AndorCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual char* alloc_data_type(Xtc& xtc);
    protected:
      char*  _cooling;
      char*  _overlap;
      char*  _noise_filter;
      char*  _mcp_intelligate;
      char*  _shutter;
      char*  _fan_speed;
      char*  _readout_rate;
      char*  _gain_mode;
      char*  _gate_mode;
      char*  _insertion_delay;
      char*  _setpoint;
      uint32_t _mcp_gain;
      double _trigger_delay;
      typename Cfg::ATBool _cooling_ddl;
      typename Cfg::ATBool _overlap_ddl;
      typename Cfg::ATBool _noise_filter_ddl;
      typename Cfg::ATBool _blemish_correction_ddl;
      typename Cfg::FanSpeed _fan_speed_ddl;
      typename Cfg::ReadoutRate _readout_rate_ddl;
      typename Cfg::TriggerMode _trigger_mode_ddl;
      typename Cfg::GainMode _gain_mode_ddl;
      Cfg _cfg;
    };

    class ZylaCamServer : public AndorCamServer<ZylaConfigType> {
    public:
      ZylaCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~ZylaCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    private:
      ZylaConfigType::ShutteringMode _shutter_ddl;
      ZylaConfigType::CoolingSetpoint _setpoint_ddl;
    };

    class iStarCamServer : public AndorCamServer<iStarConfigType> {
    public:
      iStarCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~iStarCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    private:
      iStarConfigType::ATBool _mcp_intelligate_ddl;
      iStarConfigType::GateMode _gate_mode_ddl;
      iStarConfigType::InsertionDelay _insertion_delay_ddl;
    };
  }
}

#endif
