#include "pds/pvdaq/ControlsCameraServer.hh"
#include "pds/pvdaq/PvMacros.hh"
#include "pds/config/EpicsCamConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>

using namespace Pds::PvDaq;

ControlsCamServer::ControlsCamServer(const char*    pvbase,
                                     const char*    pvimage,
                                     const DetInfo& info,
                                     const unsigned max_event_size,
                                     const unsigned flags) :
  EpicsCamServer(pvbase, pvimage, info, max_event_size, flags)
{}

ControlsCamServer::~ControlsCamServer()
{}

Pds::Xtc* ControlsCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_epicsCamConfigType, _xtc.src );
  new (xtc->alloc(sizeof(EpicsCamConfigType)))
    EpicsCamConfigType(_width, _height, _depth, EpicsCamConfigType::Mono, _exposure, _gain, _manufacturer, _model);
  dg->xtc.alloc(xtc->extent);

  return xtc;
}

RayonixCamServer::RayonixCamServer(const char*    pvbase,
                                   const char*    pvimage,
                                   const DetInfo& info,
                                   const unsigned max_event_size,
                                   const unsigned flags) :
  EpicsCamServer(pvbase, pvimage, info, max_event_size, flags),
  _rnx_bin(new char[ENUM_PV_LEN]),
  _rnx_trig(new char[ENUM_PV_LEN]),
  _rnx_mode(new char[ENUM_PV_LEN]),
  _rnx_bin_ddl(0),
  _rnx_trig_ddl(0),
  _rnx_mode_ddl(RayonixConfigType::Unknown)
{
  _rnx_bin[0] = '\0';
  _rnx_trig[0] = '\0';
  _rnx_mode[0] = '\0';

  // Rayonix binning pv
  CREATE_ENUM_PV("Bin_RBV", NULL, _rnx_bin, ENUM_PV_LEN);
  // Rayonix trigger pv
  CREATE_ENUM_PV("TriggerMode_RBV", NULL, _rnx_trig, ENUM_PV_LEN);
  // Rayonix readout mode pv
  CREATE_ENUM_PV("ReadoutMode_RBV", NULL, _rnx_mode, ENUM_PV_LEN);
}

RayonixCamServer::~RayonixCamServer()
{
  delete[] _rnx_bin;
  delete[] _rnx_trig;
  delete[] _rnx_mode;
}

bool RayonixCamServer::configure()
{
  bool error = false;

  // Convert rnx_bin to DDL value scheme
  if (!strcmp(_rnx_bin, "1x1")) {
    _rnx_bin_ddl = 1;
  } else if (!strcmp(_rnx_bin, "2x2")) {
    _rnx_bin_ddl = 2;
  } else if (!strcmp(_rnx_bin, "3x3")) {
    _rnx_bin_ddl = 3;
  } else if (!strcmp(_rnx_bin, "4x4")) {
    _rnx_bin_ddl = 4;
  } else if (!strcmp(_rnx_bin, "6x6")) {
    _rnx_bin_ddl = 6;
  } else if (!strcmp(_rnx_bin, "10x10")) {
    _rnx_bin_ddl = 10;
  } else {
    printf("Error: failed to parse rayonix binning mode enum value: %s\n", _rnx_bin);
    error = true;
  }

  // Convert rnx_trig to DDL value scheme
  if (!strcmp(_rnx_trig, "FreeRun")) {
    _rnx_trig_ddl = 0;
  } else if (!strcmp(_rnx_trig, "Edge")) {
    _rnx_trig_ddl = 1;
  } else if (!strcmp(_rnx_trig, "Bulb")) {
    _rnx_trig_ddl = 2;
  } else if (!strcmp(_rnx_trig, "LCLSMode")) {
    _rnx_trig_ddl = 3;
  } else {
    printf("Error: failed to parse rayonix trigger mode enum value: %s\n", _rnx_trig);
    error = true;
  }

  // Convert rnx_mode to DDL value scheme
  if (!strcmp(_rnx_mode, "Standard")) {
    _rnx_mode_ddl = RayonixConfigType::Standard;
  } else if (!strcmp(_rnx_mode, "LowNoise")) {
    _rnx_mode_ddl = RayonixConfigType::LowNoise;
  } else {
    printf("Error: failed to parse rayonix camera mode enum value: %s\n", _rnx_mode);
    error = true;
  }

  return error;
}

void RayonixCamServer::show_configuration()
{
  std::string rnx_mode = enum_to_str<RayonixConfigType::ReadoutMode>(_rnx_mode_ddl);

  printf("  binning mode:     %s\n"
         "  trigger mode:     %s\n"
         "  readout mode:     %s\n",
         _rnx_bin,
         _rnx_trig,
         rnx_mode.c_str());
}

Pds::Xtc* RayonixCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_rayonixConfigType, _xtc.src );
  new (xtc->alloc(sizeof(RayonixConfigType)))
    RayonixConfigType(_rnx_bin_ddl, _rnx_bin_ddl, 0, (uint32_t) _exposure, _rnx_trig_ddl, 0, 0, _rnx_mode_ddl, _model);
  dg->xtc.alloc(xtc->extent);

  return xtc;
}

const unsigned StreakCamServer::FPATH_PV_LEN = 10000;

StreakCamServer::StreakCamServer(const char*    pvbase,
                                 const char*    pvimage,
                                 const DetInfo& info,
                                 const unsigned max_event_size,
                                 const unsigned flags) :
  EpicsCamServer(pvbase, pvimage, info, max_event_size, flags),
  _strk_fto(0),
  _strk_time(new char[ENUM_PV_LEN]),
  _strk_mode(new char[ENUM_PV_LEN]),
  _strk_gate(new char[ENUM_PV_LEN]),
  _strk_shut(new char[ENUM_PV_LEN]),
  _strk_trig(new char[ENUM_PV_LEN]),
  _strk_path(new char[FPATH_PV_LEN]),
  _strk_time_ddl(0),
  _strk_mode_ddl(StreakConfigType::Focus),
  _strk_gate_ddl(StreakConfigType::Normal),
  _strk_shut_ddl(StreakConfigType::Closed),
  _strk_trig_ddl(StreakConfigType::Single),
  _strk_scale_ddl(StreakConfigType::Seconds)
{
  _strk_time[0] = '\0';
  _strk_mode[0] = '\0';
  _strk_gate[0] = '\0';
  _strk_shut[0] = '\0';
  _strk_trig[0] = '\0';
  _strk_path[0] = '\0';

  memset(_calib, 0, sizeof(_calib));

  // Streak camera time range pv
  CREATE_ENUM_PV("TimeRange_RBV", NULL, _strk_time, ENUM_PV_LEN);
  // Streak camera mode pv
  CREATE_ENUM_PV("TriggerMode_RBV", NULL, _strk_mode, ENUM_PV_LEN);
  // Streak camera gate mode pv
  CREATE_ENUM_PV("GateMode_RBV", NULL, _strk_gate, ENUM_PV_LEN);
  // Streak camera shutter mode pv
  CREATE_ENUM_PV("Shutter_RBV", NULL, _strk_shut, ENUM_PV_LEN);
  // Streak camera tigger mode pv
  CREATE_ENUM_PV("ImageMode_RBV", NULL, _strk_trig, ENUM_PV_LEN);
  // Streak camera focus time over pv
  CREATE_PV("FocusTimeOver_RBV", NULL, _strk_fto);
  // Streak camera configuration filepath pv
  CREATE_LONGSTR_PV("ScalingFilePath", NULL, _strk_path, FPATH_PV_LEN);
}

StreakCamServer::~StreakCamServer()
{
  delete[] _strk_time;
  delete[] _strk_mode;
  delete[] _strk_gate;
  delete[] _strk_shut;
  delete[] _strk_trig;
  delete[] _strk_path;
}

bool StreakCamServer::configure()
{
  bool error = false;

  // Convert strk_time to format need by DDL
  char* unit;
  double time_val = strtod(_strk_time, &unit);
  if (unit) {
    unit++;
    if (!strcmp(unit, "ns")) {
      _strk_time_ddl = (uint64_t) (time_val * 1e3);
    } else if (!strcmp(unit, "us")) {
      _strk_time_ddl = (uint64_t) (time_val * 1e6);
    } else if (!strcmp(unit, "ms")) {
      _strk_time_ddl = (uint64_t) (time_val * 1e9);
    } else if (!strcmp(unit, "s")) {
      _strk_time_ddl = (uint64_t) (time_val * 1e12);
    } else {
      printf("Error: failed to parse streak camera time range units: %s\n", _strk_time);
      error = true;
    }
  } else {
    printf("Error: failed to parse streak camera time range: %s\n", _strk_time);
    error = true;
  }

  // Convert strk_mode to DDL value scheme
  if (!strcmp(_strk_mode, "Focus")) {
    _strk_mode_ddl = StreakConfigType::Focus;
  } else if (!strcmp(_strk_mode, "Operate")) {
    _strk_mode_ddl = StreakConfigType::Operate;
  } else {
    printf("Error: failed to parse streak camera mode enum value: %s\n", _strk_mode);
    error = true;
  }

  // Convert strk_gate to DDL value scheme
  if (!strcmp(_strk_gate, "Normal")) {
    _strk_gate_ddl = StreakConfigType::Normal;
  } else if (!strcmp(_strk_gate, "Gate")) {
    _strk_gate_ddl = StreakConfigType::Gate;
  } else if (!strcmp(_strk_gate, "Open Fixed")) {
    _strk_gate_ddl = StreakConfigType::OpenFixed;
  } else {
    printf("Error: failed to parse streak camera gate enum value: %s\n", _strk_gate);
    error = true;
  }

  // Convert strk_shut to DDL value scheme
  if (!strcmp(_strk_shut, "Closed")) {
    _strk_shut_ddl = StreakConfigType::Closed;
  } else if (!strcmp(_strk_shut, "Open")) {
    _strk_shut_ddl = StreakConfigType::Open;
  } else {
    printf("Error: failed to parse streak camera shutter enum value: %s\n", _strk_shut);
    error = true;
  }

  // Convert strk_trig to DDL value scheme
  if (!strcmp(_strk_trig, "Single")) {
    _strk_trig_ddl = StreakConfigType::Single;
  } else if (!strcmp(_strk_trig, "Continuous")) {
    _strk_trig_ddl = StreakConfigType::Continuous;
  } else {
    printf("Error: failed to parse streak camera trigger enum value: %s\n", _strk_trig);
    error = true;
  }

  FILE* f = fopen(_strk_path, "r");
  if (f) {
    bool failed = true;
    int skip = 3;
    size_t line_sz = 1024;
    char* line = (char *)malloc(line_sz);

    while (getline(&line, &line_sz, f) != -1) {
      if (skip) {
        skip--;
      } else {
        char* token = strtok(line, ",");
        if (token && !strcmp(token, _strk_time)) {
          int pos = 0;
          token = strtok(NULL, ",");
          while (token != NULL) {
            if (pos == 0) {
              if (!strcmp(token, "ns"))
                _strk_scale_ddl = StreakConfigType::Nanoseconds;
              else if (!strcmp(token, "us"))
                _strk_scale_ddl = StreakConfigType::Microseconds;
              else if (!strcmp(token, "ms"))
                _strk_scale_ddl = StreakConfigType::Milliseconds;
              else if (!strcmp(token, "s"))
                _strk_scale_ddl = StreakConfigType::Seconds;
            } else if((pos > 1) && (pos < (StreakConfigType::NumCalibConstants + 2))) {
              _calib[pos - 2] = strtod(token, NULL);
            }
            token = strtok(NULL, ",");
            pos++;
          }
          failed = (pos != (StreakConfigType::NumCalibConstants + 2));
        }
      }
    }

    if (failed) {
      printf("Error: failed to parse streak camera calibration file: %s\n", _strk_path);
      error = true;
    }

    if (line) free(line);
  } else {
    printf("Error: failed to open streak camera calibration file: %s\n", _strk_path);
    error = true;
  }

  return error;
}

void StreakCamServer::show_configuration()
{
  std::string strk_mode  = enum_to_str<StreakConfigType::DeviceMode>(_strk_mode_ddl);
  std::string strk_gate  = enum_to_str<StreakConfigType::GateMode>(_strk_gate_ddl);
  std::string strk_shut  = enum_to_str<StreakConfigType::ShutterMode>(_strk_shut_ddl);
  std::string strk_trig  = enum_to_str<StreakConfigType::TriggerMode>(_strk_trig_ddl);
  std::string strk_scale = enum_to_str<StreakConfigType::CalibScale>(_strk_scale_ddl);

  printf("  time range:       %s\n"
         "  device mode:      %s\n"
         "  gate mode:        %s\n"
         "  shutter mode:     %s\n"
         "  trigger mode:     %s\n"
         "  calib scale:      %s\n"
         "  calib constants: ",
         _strk_time,
         strk_mode.c_str(),
         strk_gate.c_str(),
         strk_shut.c_str(),
         strk_trig.c_str(),
         strk_scale.c_str());
  for (int i=0; i<StreakConfigType::NumCalibConstants; i++) {
    printf(" %g", _calib[i]);
  }
  printf("\n");
}

Pds::Xtc* StreakCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_streakConfigType, _xtc.src );
  new (xtc->alloc(sizeof(StreakConfigType)))
    StreakConfigType(_strk_time_ddl,
                     _strk_mode_ddl,
                     _strk_gate_ddl,
                     (uint32_t) _gain,
                     _strk_shut_ddl,
                     _strk_trig_ddl,
                     _strk_fto,
                     _exposure,
                     _strk_scale_ddl,
                     _calib);
  dg->xtc.alloc(xtc->extent);

  return xtc;
}

ArchonCamServer::ArchonCamServer(const char*    pvbase,
                                 const char*    pvimage,
                                 const DetInfo& info,
                                 const unsigned max_event_size,
                                 const unsigned flags) :
  EpicsCamServer(pvbase, pvimage, info, max_event_size, flags),
  _arch_psweep(0),
  _arch_isweep(0),
  _arch_skip(0),
  _arch_st(0),
  _arch_stm1(0),
  _arch_at(0),
  _arch_pixels(0),
  _arch_lines(0),
  _arch_taps(0),
  _arch_batch(0),
  _arch_nonint(0.),
  _arch_volt(0.),
  _arch_pwr(new char[ENUM_PV_LEN]),
  _arch_chan(new char[ENUM_PV_LEN]),
  _arch_bias(new char[ENUM_PV_LEN]),
  _arch_lsmode(new char[ENUM_PV_LEN]),
  _arch_is_batched(false),
  _arch_pwr_ddl(ArchonConfigType::Off),
  _arch_chan_ddl(ArchonConfigType::NV1),
  _arch_bias_ddl(ArchonConfigType::Off)
{
  _arch_pwr[0] = '\0';
  _arch_chan[0] = '\0';
  _arch_bias[0] = '\0';
  _arch_lsmode[0] = '\0';

  // Archon pre-frame clear count
  CREATE_PV("ArchonPreFrameClear_RBV", NULL, _arch_psweep);
  // Archon idle clear count
  CREATE_PV("ArchonIdleClear_RBV", NULL, _arch_isweep);
  // Archon pre frame skip lines count
  CREATE_PV("ArchonPreFrameSkip_RBV", NULL, _arch_skip);
  // Archon clock st
  CREATE_PV("ArchonClockST_RBV", NULL, _arch_st);
  // Archon clock stm1
  CREATE_PV("ArchonClockSTM1_RBV", NULL, _arch_stm1);
  // Archon clock at
  CREATE_PV("ArchonClockAT_RBV", NULL, _arch_at);
  // Archon max pixels
  CREATE_PV("MaxSizeX_RBV", NULL, _arch_pixels);
  // Archon max lines
  CREATE_PV("MaxSizeY_RBV", NULL, _arch_lines);
  // Archon num tap lines
  CREATE_PV("ArchonActiveTaplines_RBV", NULL, _arch_taps);
  // Archon non-integration time
  CREATE_PV("ArchonNonIntTime_RBV", NULL, _arch_nonint);
  // Archon ccd power
  CREATE_ENUM_PV("ArchonPowerMode_RBV", NULL, _arch_pwr, ENUM_PV_LEN);
  // Archon linescan mode
  CREATE_ENUM_PV("ArchonLineScanMode_RBV", NULL, _arch_lsmode, ENUM_PV_LEN);
  // Archon number of batches per frame
  CREATE_PV("ArchonNumBatchFrames_RBV", NULL, _arch_batch);
  // Archon bias channel
  CREATE_ENUM_PV("ArchonBiasChan_RBV", NULL, _arch_chan, ENUM_PV_LEN);
  // Keep a reference to the bias channel pv
  ConfigServer* arch_chan_pv = _config_pvs.back();
  // try waiting for pv to connect to read bias chan
  int tries = 100;
  while(!arch_chan_pv->connected() && tries > 0) {
    usleep(10000);
    tries--;
  }
  if (arch_chan_pv->connected()) {
    if (arch_chan_pv->fetch() < 0) {
      printf("Error: failed to read value of bias channel pv: %s\n", arch_chan_pv->name());
    } else {
      const char* chan_name = NULL;
      if (!strcmp(_arch_chan, "NV4")) {
        chan_name = "XVBN4";
      } else if (!strcmp(_arch_chan, "NV3")) {
        chan_name = "XVBN3";
      } else if (!strcmp(_arch_chan, "NV2")) {
        chan_name = "XVBN2";
      } else if (!strcmp(_arch_chan, "NV1")) {
        chan_name = "XVBN1";
      } else if (!strcmp(_arch_chan, "PV1")) {
        chan_name = "XVBP1";
      } else if (!strcmp(_arch_chan, "PV2")) {
        chan_name = "XVBP2";
      } else if (!strcmp(_arch_chan, "PV3")) {
        chan_name = "XVBP3";
      } else if (!strcmp(_arch_chan, "PV4")) {
        chan_name = "XVBP4";
      }
      if (chan_name) {
        // Archon bias voltage
        CREATE_PV("ArchonBiasSetpoint_RBV", chan_name, _arch_volt);
        // Archon bias state
        CREATE_ENUM_PV("ArchonBiasSwitch_RBV", chan_name, _arch_bias, ENUM_PV_LEN);
      } else {
        printf("Error: failed to parse archon bias channel enum value: %s\n", arch_chan_pv->name());
      }
    }
  } else {
    printf("Error: failed to connect to bias channel pv: %s\n", arch_chan_pv->name());
  }
}

ArchonCamServer::~ArchonCamServer()
{
  delete[] _arch_pwr;
  delete[] _arch_chan;
  delete[] _arch_bias;
  delete[] _arch_lsmode;
}

bool ArchonCamServer::configure()
{
  bool error = false;

  // Convert arch_pwr to DDL value scheme
  if (!strcmp(_arch_pwr, "Off")) {
    _arch_pwr_ddl = ArchonConfigType::Off;
  } else if (!strcmp(_arch_pwr, "On")) {
    _arch_pwr_ddl = ArchonConfigType::On;
  } else {
    printf("Error: failed to parse archon power enum value: %s\n", _arch_pwr);
    error = true;
  }

  // Convert arch chan to DDL value scheme
  if (!strcmp(_arch_chan, "NV4")) {
    _arch_chan_ddl = ArchonConfigType::NV4;
  } else if (!strcmp(_arch_chan, "NV3")) {
    _arch_chan_ddl = ArchonConfigType::NV3;
  } else if (!strcmp(_arch_chan, "NV2")) {
    _arch_chan_ddl = ArchonConfigType::NV2;
  } else if (!strcmp(_arch_chan, "NV1")) {
    _arch_chan_ddl = ArchonConfigType::NV1;
  } else if (!strcmp(_arch_chan, "PV1")) {
    _arch_chan_ddl = ArchonConfigType::PV1;
  } else if (!strcmp(_arch_chan, "PV2")) {
    _arch_chan_ddl = ArchonConfigType::PV2;
  } else if (!strcmp(_arch_chan, "PV3")) {
    _arch_chan_ddl = ArchonConfigType::PV3;
  } else if (!strcmp(_arch_chan, "PV4")) {
    _arch_chan_ddl = ArchonConfigType::PV4;
  } else {
    printf("Error: failed to parse archon bias channel enum value: %s\n", _arch_chan);
    error = true;
  }

  // Convert arch_bias to DDL value scheme
  if (!strcmp(_arch_bias, "Off")) {
    _arch_bias_ddl = ArchonConfigType::Off;
  } else if (!strcmp(_arch_bias, "On")) {
    _arch_bias_ddl = ArchonConfigType::On;
  } else {
    printf("Error: failed to parse archon bias enum value: %s\n", _arch_bias);
    error = true;
  }

  // Convert arch_lsmode to DDL value scheme
  if (!strcmp(_arch_lsmode, "Disable")) {
    _arch_is_batched = false;
  } else if (!strcmp(_arch_lsmode, "Enable")) {
    _arch_is_batched = true;
  } else {
    printf("Error: failed to parse archon linescan enum value: %s\n", _arch_lsmode);
    error = true;
  }

  return error;
}

void ArchonCamServer::show_configuration()
{
  std::string arch_pwr = enum_to_str<ArchonConfigType::Switch>(_arch_pwr_ddl);
  std::string arch_bias = enum_to_str<ArchonConfigType::Switch>(_arch_bias_ddl);
  std::string arch_chan = enum_to_str<ArchonConfigType::BiasChannelId>(_arch_chan_ddl);

  printf("  ccd power:        %s\n"
         "  ccd bias:         %s\n"
         "  ccd bias chan:    %s\n"
         "  ccd bias volt:    %f\n"
         "  non-int time:     %f\n"
         "  pre-frame clear:  %u\n"
         "  idle clear:       %u\n"
         "  pre-frame skip:   %u\n"
         "  clock at:         %u\n"
         "  clock st:         %u\n"
         "  clock stm1:       %u\n"
         "  sensor pixels:    %u\n"
         "  sensor lines:     %u\n"
         "  sensor taps:      %u\n"
         "  linescan mode:    %s\n"
         "  num batches:      %u\n",
         arch_pwr.c_str(), arch_bias.c_str(),
         arch_chan.c_str(), _arch_volt,
         _arch_nonint,
         _arch_psweep, _arch_isweep,
         _arch_skip, _arch_at,
         _arch_st, _arch_stm1,
         _arch_pixels, _arch_lines,
         _arch_taps, _arch_lsmode,
         _arch_is_batched ? _arch_batch : 0);
}

Pds::Xtc* ArchonCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_archonConfigType, _xtc.src );
  new (xtc->alloc(sizeof(ArchonConfigType)))
    ArchonConfigType(ArchonConfigType::Triggered,
                     _arch_pwr_ddl,
                     0, // exposure event code - don't use
                     0, // config size - don't attach acf so this is zero
                     _arch_psweep,
                     _arch_isweep,
                     _arch_skip,
                     (uint32_t) (_exposure*1000), // needs to be in ms as int
                     (uint32_t) (_arch_nonint*1000), // needs to be in ms as int
                     _arch_is_batched ? _arch_batch : 0,
                     _width / _arch_taps,
                     _height,
                     _arch_pixels / _width,
                     _arch_lines / _height,
                     _arch_pixels / _arch_taps,
                     _arch_lines,
                     _arch_taps,
                     _arch_st,
                     _arch_stm1,
                     _arch_at,
                     _arch_bias_ddl,
                     _arch_chan_ddl,
                     _arch_volt,
                     0, // config version - don't use
                     NULL);
  dg->xtc.alloc(xtc->extent);

  return xtc;
}
