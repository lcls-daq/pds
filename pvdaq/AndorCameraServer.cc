#include "pds/pvdaq/AndorCameraServer.hh"
#include "pds/pvdaq/PvMacros.hh"
#include "pds/config/ZylaDataType.hh"
#include "pds/config/EpicsCamConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>
#include <sstream>

using namespace Pds::PvDaq;

template <typename Cfg>
bool convert_enum_atbool(char* value, typename Cfg::ATBool& value_ddl)
{
  bool error = false;

  if (!strcmp(value, "Off")) {
    value_ddl = Cfg::False;
  } else if (!strcmp(value, "On")) {
    value_ddl = Cfg::True;
  } else if (!strcmp(value, "No")) {
    value_ddl = Cfg::False;
  } else if (!strcmp(value, "Yes")) {
    value_ddl = Cfg::True;
  } else {
    error = true;
  }

  return error;
}

template <typename Cfg>
AndorCamServer<Cfg>::AndorCamServer(const char*    pvbase,
                               const char*    pvimage,
                               const DetInfo& info,
                               const unsigned max_event_size,
                               const unsigned flags) :
  AreaDetectorServer(pvbase, pvimage, info, max_event_size, flags),
  _cooling(new char[ENUM_PV_LEN]),
  _overlap(new char[ENUM_PV_LEN]),
  _noise_filter(new char[ENUM_PV_LEN]),
  _mcp_intelligate(new char[ENUM_PV_LEN]),
  _shutter(new char[ENUM_PV_LEN]),
  _fan_speed(new char[ENUM_PV_LEN]),
  _readout_rate(new char[ENUM_PV_LEN]),
  _gain_mode(new char[ENUM_PV_LEN]),
  _gate_mode(new char[ENUM_PV_LEN]),
  _insertion_delay(new char[ENUM_PV_LEN]),
  _setpoint(new char[ENUM_PV_LEN]),
  _mcp_gain(0),
  _trigger_delay(0.),
  _cooling_ddl(Cfg::False),
  _overlap_ddl(Cfg::False),
  _noise_filter_ddl(Cfg::False),
  _blemish_correction_ddl(Cfg::False),
  _fan_speed_ddl(Cfg::Off),
  _readout_rate_ddl(Cfg::Rate280MHz),
  _trigger_mode_ddl(Cfg::Internal),
  _gain_mode_ddl(Cfg::HighWellCap12Bit)
{
  _xtc = Xtc(_zylaDataType, info);
  _cooling[0]            = '\0';
  _overlap[0]            = '\0';
  _noise_filter[0]       = '\0';
  _mcp_intelligate[0]    = '\0';
  _shutter[0]            = '\0';
  _fan_speed[0]          = '\0';
  _readout_rate[0]       = '\0';
  _gain_mode[0]          = '\0';
  _gate_mode[0]          = '\0';
  _insertion_delay[0]    = '\0';
  _setpoint[0]           = '\0';

  // cooling enable pv
  CREATE_ENUM_PV("SensorCooling_RBV", NULL, _cooling, ENUM_PV_LEN);
  // overlap readout pv
  CREATE_ENUM_PV("Overlap_RBV", NULL, _overlap, ENUM_PV_LEN);
  // noise filter pv
  CREATE_ENUM_PV("NoiseFilter_RBV", NULL, _noise_filter, ENUM_PV_LEN);
  // mcp intelligate pv
  CREATE_ENUM_PV("MCPIntelligate_RBV", NULL, _mcp_intelligate, ENUM_PV_LEN);
  // shutter pv
  CREATE_ENUM_PV("A3ShutterMode_RBV", NULL, _shutter, ENUM_PV_LEN);
  // fan speed pv
  CREATE_ENUM_PV("FanSpeed_RBV", NULL, _fan_speed, ENUM_PV_LEN);
  // readout rate pv
  CREATE_ENUM_PV("ReadoutRate_RBV", NULL, _readout_rate, ENUM_PV_LEN);
  // gain mode pv
  CREATE_ENUM_PV("PreAmpGain_RBV", NULL, _gain_mode, ENUM_PV_LEN);
  // gate mode pv
  CREATE_ENUM_PV("GateMode_RBV", NULL, _gate_mode, ENUM_PV_LEN);
  // insertion delay pv
  CREATE_ENUM_PV("InsertionDelay_RBV", NULL, _insertion_delay, ENUM_PV_LEN);
  // setpoint pv
  CREATE_ENUM_PV("TempControl_RBV", NULL, _setpoint, ENUM_PV_LEN);
  // mcp gain pv
  CREATE_PV("MCPGain_RBV", NULL, _mcp_gain);
}

template <typename Cfg>
AndorCamServer<Cfg>::~AndorCamServer()
{
  delete[] _cooling;
  delete[] _overlap;
  delete[] _noise_filter;
  delete[] _mcp_intelligate;
  delete[] _shutter;
  delete[] _fan_speed;
  delete[] _readout_rate;
  delete[] _gain_mode;
  delete[] _gate_mode;
  delete[] _insertion_delay;
  delete[] _setpoint;
}

template <typename Cfg>
bool AndorCamServer<Cfg>::configure()
{
  bool error = false;

  // convert cooling pv enum to ddl version
  if (convert_enum_atbool<Cfg>(_cooling, _cooling_ddl)) {
    printf("Error: failed to parse cooling enum value: %s\n", _cooling);
    error = true;
  }

  // convert overlap pv enum to ddl version
  if (convert_enum_atbool<Cfg>(_overlap, _overlap_ddl)) {
    printf("Error: failed to parse overlap enum value: %s\n", _overlap);
    error = true;
  }

  // convert noise filter pv enum to ddl version
  if (convert_enum_atbool<Cfg>(_noise_filter, _noise_filter_ddl)) {
    printf("Error: failed to parse noise filter enum value: %s\n", _noise_filter);
    error = true;
  }

  // convert fan speed pv enum to ddl version
  if (!strcmp(_fan_speed, "Off")) {
    _fan_speed_ddl = Cfg::Off;
  } else if (!strcmp(_fan_speed, "On")) {
    _fan_speed_ddl = Cfg::On;
  } else {
    printf("Error: failed to parse fan speed enum value: %s\n", _fan_speed);
    error = true;
  }

  // convert readout rate pv enum to ddl version
  if (!strcmp(_readout_rate, "100 MHz")) {
    _readout_rate_ddl = Cfg::Rate100MHz;
  } else if (!strcmp(_readout_rate, "280 MHz")) {
    _readout_rate_ddl = Cfg::Rate280MHz;
  } else {
    printf("Error: failed to parse readout rate enum value: %s\n", _readout_rate);
    error = true;
  }

  // convert trigger mode pv enum to ddl version
  if (!strcmp(_trigger_mode, "Internal")) {
    _trigger_mode_ddl = Cfg::Internal;
  } else if (!strcmp(_trigger_mode, "External Start")) {
    _trigger_mode_ddl = Cfg::ExternalStart;
  } else if (!strcmp(_trigger_mode, "External Exposure")) {
    _trigger_mode_ddl = Cfg::ExternalExposure;
  } else if (!strcmp(_trigger_mode, "Software")) {
    _trigger_mode_ddl = Cfg::Software;
  } else if (!strcmp(_trigger_mode, "External")) {
    _trigger_mode_ddl = Cfg::External;
  } else {
    printf("Error: failed to parse trigger mode enum value: %s\n", _trigger_mode);
    error = true;
  }

  // convert gain mode pv enum to ddl version
  if (!strcmp(_gain_mode, "12-bit (high well capacit")) {
    _gain_mode_ddl = Cfg::HighWellCap12Bit;
  } else if (!strcmp(_gain_mode, "12-bit (low noise)")) {
    _gain_mode_ddl = Cfg::LowNoise12Bit;
  } else if (!strcmp(_gain_mode, "16-bit (low noise & high")) {
    _gain_mode_ddl = Cfg::LowNoiseHighWellCap16Bit;
  } else {
    printf("Error: failed to parse gain mode enum value: %s\n", _gain_mode);
    error = true;
  }

  return error;
}

template <typename Cfg>
void AndorCamServer<Cfg>::show_configuration()
{
  std::string cooling_ddl_str = enum_to_str<typename Cfg::ATBool>(_cooling_ddl);
  std::string overlap_ddl_str = enum_to_str<typename Cfg::ATBool>(_overlap_ddl);
  std::string noise_filter_ddl_str = enum_to_str<typename Cfg::ATBool>(_noise_filter_ddl);
  std::string blemish_correction_ddl_str = enum_to_str<typename Cfg::ATBool>(_blemish_correction_ddl);
  std::string fan_speed_ddl_str = enum_to_str<typename Cfg::FanSpeed>(_fan_speed_ddl);
  std::string readout_rate_ddl_str = enum_to_str<typename Cfg::ReadoutRate>(_readout_rate_ddl);
  std::string trigger_mode_ddl_str = enum_to_str<typename Cfg::TriggerMode>(_trigger_mode_ddl);
  std::string gain_mode_ddl_str = enum_to_str<typename Cfg::GainMode>(_gain_mode_ddl);

  printf("  cooling:          %s\n"
         "  overlap:          %s\n"
         "  noise filter:     %s\n"
         "  blemish correct:  %s\n"
         "  fan speed:        %s\n"
         "  readout rate:     %s\n"
         "  trigger mode:     %s\n"
         "  gain mode:        %s\n"
         "  trigger delay:    %f\n",
         cooling_ddl_str.c_str(),
         overlap_ddl_str.c_str(),
         noise_filter_ddl_str.c_str(),
         blemish_correction_ddl_str.c_str(),
         fan_speed_ddl_str.c_str(),
         readout_rate_ddl_str.c_str(),
         trigger_mode_ddl_str.c_str(),
         gain_mode_ddl_str.c_str(),
         _trigger_delay);
}

template <typename Cfg>
char* AndorCamServer<Cfg>::alloc_data_type(Xtc& xtc)
{
  ZylaDataType* frameptr = new (xtc.alloc(sizeof(ZylaDataType)))
    ZylaDataType(0);
  xtc.alloc(frameptr->_sizeof(_cfg)-sizeof(ZylaDataType));
  char* dataptr = ((char*) frameptr) + sizeof(ZylaDataType);
  return dataptr;
}


ZylaCamServer::ZylaCamServer(const char*    pvbase,
                             const char*    pvimage,
                             const DetInfo& info,
                             const unsigned max_event_size,
                             const unsigned flags) :
  AndorCamServer<ZylaConfigType>(pvbase, pvimage, info, max_event_size, flags),
  _shutter_ddl(ZylaConfigType::Rolling),
  _setpoint_ddl(ZylaConfigType::Temp_0C)
{}

ZylaCamServer::~ZylaCamServer()
{}


bool ZylaCamServer::configure()
{
  bool error = AndorCamServer<ZylaConfigType>::configure();

  // convert shutter mode pv enum to ddl version
  if (!strcmp(_shutter, "Rolling")) {
    _shutter_ddl = ZylaConfigType::Rolling;
  } else if (!strcmp(_shutter, "Global")) {
    _shutter_ddl = ZylaConfigType::Global;
  } else {
    printf("Error: failed to parse shutter mode enum value: %s\n", _shutter);
    error = true;
  }

  // convert setpoint pv enum to ddl version
  if (!strcmp(_setpoint, "0.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_0C;
  } else if (!strcmp(_setpoint, "-5.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg5C;
  } else if (!strcmp(_setpoint, "-10.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg10C;
  } else if (!strcmp(_setpoint, "-15.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg15C;
  } else if (!strcmp(_setpoint, "-20.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg20C;
  } else if (!strcmp(_setpoint, "-25.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg25C;
  } else if (!strcmp(_setpoint, "-30.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg30C;
  } else if (!strcmp(_setpoint, "-35.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg35C;
  } else if (!strcmp(_setpoint, "-40.00")) {
    _setpoint_ddl = ZylaConfigType::Temp_Neg40C;
  } else {
    printf("Error: failed to parse cooling setpoint enum value: %s\n", _setpoint);
    error = true;
  }

  return error;
}

void ZylaCamServer::show_configuration()
{
  // Call the base andor show_configuration
  AndorCamServer<ZylaConfigType>::show_configuration();

  // show zyla specific info
  std::string shutter_ddl_str = enum_to_str<ZylaConfigType::ShutteringMode>(_shutter_ddl);
  std::string setpoint_ddl_str = enum_to_str<ZylaConfigType::CoolingSetpoint>(_setpoint_ddl);

  printf("  shutter:          %s\n"
         "  setpoint:         %s\n",
         shutter_ddl_str.c_str(),
         setpoint_ddl_str.c_str());
}

Pds::Xtc* ZylaCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_zylaConfigType, _xtc.src);
  ZylaConfigType* cfg = new (xtc->alloc(sizeof(ZylaConfigType)))
    ZylaConfigType(_cooling_ddl,
                   _overlap_ddl,
                   _noise_filter_ddl,
                   _blemish_correction_ddl,
                   _shutter_ddl,
                   _fan_speed_ddl,
                   _readout_rate_ddl,
                   _trigger_mode_ddl,
                   _gain_mode_ddl,
                   _setpoint_ddl,
                   _width,
                   _height,
                   _roi_x_org,
                   _roi_y_org,
                   _bin_x,
                   _bin_y,
                   _exposure,
                   _trigger_delay);

  dg->xtc.alloc(xtc->extent);

  // cache a copy of the config object its needed to construct data xtc
  _cfg = *cfg;

  return xtc;
}

iStarCamServer::iStarCamServer(const char*    pvbase,
                               const char*    pvimage,
                               const DetInfo& info,
                               const unsigned max_event_size,
                               const unsigned flags) :
  AndorCamServer<iStarConfigType>(pvbase, pvimage, info, max_event_size, flags),
  _mcp_intelligate_ddl(iStarConfigType::False),
  _gate_mode_ddl(iStarConfigType::CWOff),
  _insertion_delay_ddl(iStarConfigType::Normal)
{}

iStarCamServer::~iStarCamServer()
{}


bool iStarCamServer::configure()
{
  bool error = AndorCamServer<iStarConfigType>::configure();

  // convert mcp intelligate pv enum to ddl version
  if(convert_enum_atbool<iStarConfigType>(_mcp_intelligate, _mcp_intelligate_ddl)) {
    printf("Error: failed to parse mcp intelligate enum value: %s\n", _mcp_intelligate);
    error = true;
  }

  // convert gate mode pv enum to ddl version
  if (!strcmp(_gate_mode, "CW On")) {
    _gate_mode_ddl = iStarConfigType::CWOn;
  } else if (!strcmp(_shutter, "CW Off")) {
    _gate_mode_ddl = iStarConfigType::CWOff;
  } else if (!strcmp(_shutter, "Fire Only")) {
    _gate_mode_ddl = iStarConfigType::FireOnly;
  } else if (!strcmp(_shutter, "Gate Only")) {
    _gate_mode_ddl = iStarConfigType::GateOnly;
  } else if (!strcmp(_shutter, "Fire and Gate")) {
    _gate_mode_ddl = iStarConfigType::FireAndGate;
  } else if (!strcmp(_shutter, "DDG")) {
    _gate_mode_ddl = iStarConfigType::DDG;
  } else {
    printf("Error: failed to parse gate mode enum value: %s\n", _gate_mode);
    error = true;
  }

  // convert insertion delay pv enum to ddl version
  if (!strcmp(_insertion_delay, "Normal")) {
    _insertion_delay_ddl = iStarConfigType::Normal;
  } else if (!strcmp(_insertion_delay, "Fast")) {
    _insertion_delay_ddl = iStarConfigType::Fast;
  } else {
    printf("Error: failed to parse insertion delay enum value: %s\n", _insertion_delay);
    error = true;
  }

  return error;
}

void iStarCamServer::show_configuration()
{
  // Call the base andor show_configuration
  AndorCamServer<iStarConfigType>::show_configuration();

  // show istar specific info
  std::string mcp_intelligate_ddl_str = enum_to_str<iStarConfigType::ATBool>(_mcp_intelligate_ddl);
  std::string gate_mode_ddl_str = enum_to_str<iStarConfigType::GateMode>(_gate_mode_ddl);
  std::string insertion_delay_ddl_str = enum_to_str<iStarConfigType::InsertionDelay>(_insertion_delay_ddl);

  printf("  mcp intelligate:  %s\n"
         "  gate mode:        %s\n"
         "  insertion delay:  %s\n"
         "  mcp gain:         %u\n",
         mcp_intelligate_ddl_str.c_str(),
         gate_mode_ddl_str.c_str(),
         insertion_delay_ddl_str.c_str(),
         _mcp_gain);
}

Pds::Xtc* iStarCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_zylaConfigType, _xtc.src);
  iStarConfigType* cfg = new (xtc->alloc(sizeof(iStarConfigType)))
    iStarConfigType(_cooling_ddl,
                    _overlap_ddl,
                    _noise_filter_ddl,
                    _blemish_correction_ddl,
                    _mcp_intelligate_ddl,
                    _fan_speed_ddl,
                    _readout_rate_ddl,
                    _trigger_mode_ddl,
                    _gain_mode_ddl,
                    _gate_mode_ddl,
                    _insertion_delay_ddl,
                    (uint16_t) _mcp_gain,
                    _width,
                    _height,
                    _roi_x_org,
                    _roi_y_org,
                    _bin_x,
                    _bin_y,
                    _exposure,
                    _trigger_delay);

  dg->xtc.alloc(xtc->extent);

  // cache a copy of the config object its needed to construct data xtc
  _cfg = *cfg;

  return xtc;
}
