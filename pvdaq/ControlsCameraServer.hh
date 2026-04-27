#ifndef Pds_PvDaq_ControlsCameraServer_hh
#define Pds_PvDaq_ControlsCameraServer_hh

#include "pds/pvdaq/AreaDetectorServer.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/config/StreakConfigType.hh"
#include "pds/config/ArchonConfigType.hh"

namespace Pds {
  namespace PvDaq {
    class ControlsCamServer : public EpicsCamServer {
    public:
      ControlsCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~ControlsCamServer();
    protected:
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    };

    class RayonixCamServer : public EpicsCamServer {
    public:
      RayonixCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~RayonixCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    private:
      char*  _rnx_bin;
      char*  _rnx_trig;
      char*  _rnx_mode;
      uint8_t   _rnx_bin_ddl;
      uint32_t  _rnx_trig_ddl;
      RayonixConfigType::ReadoutMode _rnx_mode_ddl;
    };

    class StreakCamServer : public EpicsCamServer {
    public:
      StreakCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~StreakCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    private:
      static const unsigned FPATH_PV_LEN;
      uint32_t _strk_fto;
      char*    _strk_time;
      char*    _strk_mode;
      char*    _strk_gate;
      char*    _strk_shut;
      char*    _strk_trig;
      char*    _strk_path;
      uint64_t _strk_time_ddl;
      StreakConfigType::DeviceMode  _strk_mode_ddl;
      StreakConfigType::GateMode    _strk_gate_ddl;
      StreakConfigType::ShutterMode _strk_shut_ddl;
      StreakConfigType::TriggerMode _strk_trig_ddl;
      StreakConfigType::CalibScale  _strk_scale_ddl;
      double _calib[StreakConfigType::NumCalibConstants];
    };

    class ArchonCamServer : public EpicsCamServer {
    public:
      ArchonCamServer(const char*, const char*, const DetInfo&, const unsigned, const unsigned);
      virtual ~ArchonCamServer();
    protected:
      virtual bool configure();
      virtual void show_configuration();
      virtual Xtc* alloc_config_type(Pds::InDatagram* dg);
    private:
      uint32_t _arch_psweep;
      uint32_t _arch_isweep;
      uint32_t _arch_skip;
      uint32_t _arch_st;
      uint32_t _arch_stm1;
      uint32_t _arch_at;
      uint32_t _arch_pixels;
      uint32_t _arch_lines;
      uint32_t _arch_taps;
      uint32_t _arch_batch;
      double   _arch_nonint;
      double   _arch_volt;
      char*    _arch_pwr;
      char*    _arch_chan;
      char*    _arch_bias;
      char*    _arch_lsmode;
      bool     _arch_is_batched;
      ArchonConfigType::Switch _arch_pwr_ddl;
      ArchonConfigType::BiasChannelId _arch_chan_ddl;
      ArchonConfigType::Switch _arch_bias_ddl;
    };
  }
}

#endif
