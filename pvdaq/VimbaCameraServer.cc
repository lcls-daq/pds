#include "pds/pvdaq/VimbaCameraServer.hh"
#include "pds/pvdaq/PvMacros.hh"
#include "pds/config/VimbaDataType.hh"
#include "pds/config/EpicsCamConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include <stdio.h>

using namespace Pds::PvDaq;

AlviumCamServer::AlviumCamServer(const char*    pvbase,
                                 const char*    pvimage,
                                 const DetInfo& info,
                                 const unsigned max_event_size,
                                 const unsigned flags) :
  AreaDetectorServer(pvbase, pvimage, info, max_event_size, flags),
  _reverse_x(new char[ENUM_PV_LEN]),
  _reverse_y(new char[ENUM_PV_LEN]),
  _contrast(new char[ENUM_PV_LEN]),
  _correction(new char[ENUM_PV_LEN]),
  _correction_type(new char[ENUM_PV_LEN]),
  _correction_set(new char[ENUM_PV_LEN]),
  _pixel_mode(new char[ENUM_PV_LEN]),
  _contrast_dark_limit(0.),
  _contrast_bright_limit(0.),
  _contrast_shape(0.),
  _black_level(0.),
  _gamma(0.),
  _family(new char[ENUM_PV_LEN]),
  _manufacturer_id(new char[ENUM_PV_LEN]),
  _dev_version(new char[ENUM_PV_LEN]),
  _firmware_id(new char[ENUM_PV_LEN]),
  _reverse_x_ddl(AlviumConfigType::False),
  _reverse_y_ddl(AlviumConfigType::False),
  _contrast_ddl(AlviumConfigType::False),
  _correction_ddl(AlviumConfigType::False),
  _roi_mode_ddl(AlviumConfigType::Off),
  _correction_type_ddl(AlviumConfigType::DefectPixelCorrection),
  _correction_set_ddl(AlviumConfigType::Preset),
  _pixel_mode_ddl(AlviumConfigType::Mono8),
  _trigger_mode_ddl(AlviumConfigType::FreeRun)
{
  _xtc = Xtc(_vimbaDataType, info);
  _reverse_x[0] = '\0';
  _reverse_y[0] = '\0';
  _contrast[0] = '\0';
  _correction[0] = '\0';
  _correction_type[0] = '\0';
  _correction_set[0] = '\0';
  _pixel_mode[0] = '\0';
  _family[0] = '\0';
  _manufacturer_id[0] = '\0';
  _dev_version[0] = '\0';
  _firmware_id[0] = '\0';

  // Alvium reverse x
  CREATE_ENUM_PV("GC_ReverseX_RBV", NULL, _reverse_x, ENUM_PV_LEN);
  // Alvium reverse y
  CREATE_ENUM_PV("GC_ReverseY_RBV", NULL, _reverse_y, ENUM_PV_LEN);
  // Alvium contrast enable
  CREATE_ENUM_PV("GC_ContrastEnable_RBV", NULL, _contrast, ENUM_PV_LEN);
  // Alvium correction enable
  CREATE_ENUM_PV("GC_CorrectionMode_RBV", NULL, _correction, ENUM_PV_LEN);
  // Alvium correction type
  CREATE_ENUM_PV("GC_CorSelector_RBV", NULL, _correction_type, ENUM_PV_LEN);
  // Alvium correction set
  CREATE_ENUM_PV("GC_CorrectionSet_RBV", NULL, _correction_set, ENUM_PV_LEN);
  // Alvium pixel mode/format
  CREATE_ENUM_PV("PixelFormat_RBV", NULL, _pixel_mode, ENUM_PV_LEN);
  // Alvium contrast dark limit
  CREATE_PV("GC_ContrastDarkLimit_RBV", NULL, _contrast_dark_limit);
  // Alvium contrast bright limit
  CREATE_PV("GC_ConBrightLimit_RBV", NULL, _contrast_bright_limit);
  // Alvium contrast shape
  CREATE_PV("GC_ContrastShape_RBV", NULL, _contrast_shape);
  // Alvium black level
  CREATE_PV("GC_BlackLevel_RBV", NULL, _black_level);
  // Alvium gamma
  CREATE_PV("GC_Gamma_RBV", NULL, _gamma);
  // Alvium camera family
  CREATE_STR_PV("GC_DeviceFamilyName_RBV", NULL, _family, EpicsCamConfigType::DESC_CHAR_MAX);
  // Alvium manufacturer id
  CREATE_STR_PV("GC_DevManInfo_RBV", NULL, _manufacturer_id, EpicsCamConfigType::DESC_CHAR_MAX);
  // Alvium device id
  CREATE_STR_PV("GC_DeviceVersion_RBV", NULL, _dev_version, EpicsCamConfigType::DESC_CHAR_MAX);
  // Alvium firmware id
  CREATE_STR_PV("GC_DeviceFirmwareID_RBV", NULL, _firmware_id, EpicsCamConfigType::DESC_CHAR_MAX);
}

AlviumCamServer::~AlviumCamServer()
{
  delete[] _reverse_x;
  delete[] _reverse_y;
  delete[] _contrast;
  delete[] _correction;
  delete[] _correction_type;
  delete[] _correction_set;
  delete[] _pixel_mode;
  delete[] _family;
  delete[] _manufacturer_id;
  delete[] _dev_version;
  delete[] _firmware_id;
}

bool AlviumCamServer::configure()
{
  bool error = false;

  // Convert reverse_x to DDL value scheme
  if (!strcmp(_reverse_x, "No")) {
    _reverse_x_ddl = AlviumConfigType::False;
  } else if (!strcmp(_reverse_x, "Yes")) {
    _reverse_x_ddl = AlviumConfigType::True;
  } else {
    printf("Error: failed to parse alvium reverse x enum value: %s\n", _reverse_x);
    error = true;
  }

  // Convert reverse_x to DDL value scheme
  if (!strcmp(_reverse_y, "No")) {
    _reverse_y_ddl = AlviumConfigType::False;
  } else if (!strcmp(_reverse_y, "Yes")) {
    _reverse_y_ddl = AlviumConfigType::True;
  } else {
    printf("Error: failed to parse alvium reverse y enum value: %s\n", _reverse_y);
    error = true;
  }

  // Convert contrast to DDL value scheme
  if (!strcmp(_contrast, "No")) {
    _contrast_ddl = AlviumConfigType::False;
  } else if (!strcmp(_contrast, "Yes")) {
    _contrast_ddl = AlviumConfigType::True;
  } else {
    printf("Error: failed to parse alvium contrast enum value: %s\n", _contrast);
    error = true;
  }

  // Convert correction to DDL value scheme
  if (!strcmp(_correction, "No")) {
    _correction_ddl = AlviumConfigType::False;
  } else if (!strcmp(_correction, "Yes")) {
    _correction_ddl = AlviumConfigType::True;
  } else if (!strcmp(_correction, "")) {
    // some alvium firmwares don't have this feature - treat as off
    _correction_ddl = AlviumConfigType::False;
  } else {
    printf("Error: failed to parse alvium correction enum value: %s\n", _correction);
    error = true;
  }

  // Calculate roi_mode DDL value
  if ((_width == _max_width) && (_height == _max_height)) {
    _roi_mode_ddl = AlviumConfigType::Off;
  } else if (((_max_width - _width) == (2 * _roi_x_org)) && ((_max_height - _height) == (2 * _roi_y_org))) {
    _roi_mode_ddl = AlviumConfigType::Centered;
  } else {
    _roi_mode_ddl = AlviumConfigType::On;
  }

  // Convert correction type to DDL value scheme
  if (!strcmp(_correction_type, "Defect Pixel Correction")) {
    _correction_type_ddl = AlviumConfigType::DefectPixelCorrection;
  } else if (!strcmp(_correction_type, "Fixed Pattern Noise Corre")) {
    _correction_type_ddl = AlviumConfigType::FixedPatternNoiseCorrection;
  } else {
    printf("Error: failed to parse alvium correction type enum value: %s\n", _correction_type);
    error = true;
  }

  // Convert correction set to DDL value scheme
  if (!strcmp(_correction_set, "Preset")) {
    _correction_set_ddl = AlviumConfigType::Preset;
  } else if (!strcmp(_correction_set, "User")) {
    _correction_set_ddl = AlviumConfigType::User;
  } else if (!strcmp(_correction_set, "")) {
    // some alvium firmwares don't have this feature - treat as preset
    _correction_set_ddl = AlviumConfigType::Preset;
  } else {
    printf("Error: failed to parse alvium correction set enum value: %s\n", _correction_set);
    error = true;
  }

  // Convert pixel_mode to DDL value scheme
  if (!strcmp(_pixel_mode, "Mono8")) {
    _pixel_mode_ddl = AlviumConfigType::Mono8;
  } else if (!strcmp(_pixel_mode, "Mono10")) {
    _pixel_mode_ddl = AlviumConfigType::Mono10;
  } else if (!strcmp(_pixel_mode, "Mono10p")) {
    _pixel_mode_ddl = AlviumConfigType::Mono10p;
  } else if (!strcmp(_pixel_mode, "Mono12")) {
    _pixel_mode_ddl = AlviumConfigType::Mono12;
  } else if (!strcmp(_pixel_mode, "Mono12p")) {
    _pixel_mode_ddl = AlviumConfigType::Mono12p;
  } else {
    printf("Error: failed to parse alvium pixel mode enum value: %s\n", _pixel_mode);
    error = true;
  }

  // Convert trigger_mode to DDL value scheme
  if (!strcmp(_trigger_mode, "Off")) {
    _trigger_mode_ddl = AlviumConfigType::FreeRun;
  } else if (!strcmp(_trigger_mode, "On")) {
    _trigger_mode_ddl = AlviumConfigType::External;
  } else {
    printf("Error: failed to parse alvium trigger mode enum value: %s\n", _trigger_mode);
    error = true;
  }

  return error;
}

void AlviumCamServer::show_configuration()
{
  std::string reverse_x = enum_to_str<AlviumConfigType::VmbBool>(_reverse_x_ddl);
  std::string reverse_y = enum_to_str<AlviumConfigType::VmbBool>(_reverse_y_ddl);
  std::string contrast = enum_to_str<AlviumConfigType::VmbBool>(_contrast_ddl);
  std::string correction = enum_to_str<AlviumConfigType::VmbBool>(_correction_ddl);
  std::string correction_type = enum_to_str<AlviumConfigType::ImgCorrectionType>(_correction_type_ddl);
  std::string correction_set = enum_to_str<AlviumConfigType::ImgCorrectionSet>(_correction_set_ddl);
  std::string roi_mode = enum_to_str<AlviumConfigType::RoiMode>(_roi_mode_ddl);
  std::string pixel_mode = enum_to_str<AlviumConfigType::PixelMode>(_pixel_mode_ddl);
  std::string trigger_mode = enum_to_str<AlviumConfigType::TriggerMode>(_trigger_mode_ddl);

  printf("  reverse x:        %s\n"
         "  reverse y:        %s\n"
         "  contrast:         %s\n"
         "  correction:       %s\n"
         "  correction type:  %s\n"
         "  correction set:   %s\n"
         "  roi mode:         %s\n"
         "  pixel format:     %s\n"
         "  trigger mode:     %s\n"
         "  con dark limit:   %u\n"
         "  con bright limit: %u\n"
         "  constrast shape:  %u\n"
         "  black level:      %f\n"
         "  gamma:            %f\n"
         "  camera family:    %s\n"
         "  manufacturer id:  %s\n"
         "  device id:        %s\n"
         "  firmware id:      %s\n",
         reverse_x.c_str(),
         reverse_y.c_str(),
         contrast.c_str(),
         correction.c_str(),
         correction_type.c_str(),
         correction_set.c_str(),
         roi_mode.c_str(),
         pixel_mode.c_str(),
         trigger_mode.c_str(),
         (uint32_t) _contrast_dark_limit,
         (uint32_t) _contrast_bright_limit,
         (uint32_t) _contrast_shape,
         _black_level,
         _gamma,
         _family,
         _manufacturer_id,
         _dev_version,
         _firmware_id);
}

Pds::Xtc* AlviumCamServer::alloc_config_type(Pds::InDatagram* dg)
{
  Xtc* xtc = new ((char*)dg->xtc.next())
    Pds::Xtc(_alviumConfigType, _xtc.src);
  AlviumConfigType* cfg = new (xtc->alloc(sizeof(AlviumConfigType)))
    AlviumConfigType(_reverse_x_ddl,
                     _reverse_y_ddl,
                     _contrast_ddl,
                     _correction_ddl,
                     _roi_mode_ddl,
                     _correction_type_ddl,
                     _correction_set_ddl,
                     _pixel_mode_ddl,
                     _trigger_mode_ddl,
                     _width,
                     _height,
                     _roi_x_org,
                     _roi_y_org,
                     _max_width,
                     _max_height,
                     (uint32_t) _contrast_dark_limit,
                     (uint32_t) _contrast_bright_limit,
                     (uint32_t) _contrast_shape,
                     _exposure,
                     _black_level,
                     _gain,
                     _gamma,
                     _manufacturer,
                     _family,
                     _model,
                     _manufacturer_id,
                     _dev_version,
                     _serial_num,
                     _firmware_id,
                     _firmware_ver);
  dg->xtc.alloc(xtc->extent);

  // cache a copy of the config object its needed to construct data xtc
  _cfg = *cfg;

  return xtc;
}

char* AlviumCamServer::alloc_data_type(Xtc& xtc)
{
  VimbaDataType* frameptr = new (xtc.alloc(sizeof(VimbaDataType)))
    VimbaDataType(0, 0);
  xtc.alloc(frameptr->_sizeof(_cfg)-sizeof(VimbaDataType));
  char* dataptr = ((char*) frameptr) + sizeof(VimbaDataType);
  return dataptr;
}

