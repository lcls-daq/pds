#include "AlviumConfigCache.hh"
#include "Driver.hh"
#include "Errors.hh"

#include "pdsdata/xtc/DetInfo.hh"

#define convert_enum_case(name) \
  case AlviumConfigType::name:  \
    return Camera::name;

#define convert_enum_default(name) \
  default:                         \
    return Camera::Unsupported##name;

using namespace Pds::Vimba;

static VmbBool_t convert(AlviumConfigType::VmbBool value)
{
  switch(value) {
  case AlviumConfigType::True:
    return VmbBoolTrue;
  case AlviumConfigType::False:
    return VmbBoolFalse;
  default:
    return VmbBoolFalse;
  }
}

static Camera::TriggerMode convert(AlviumConfigType::TriggerMode trigger)
{
  switch(trigger) {
    convert_enum_case(FreeRun);
    convert_enum_case(External);
    convert_enum_case(Software);
    convert_enum_default(Trigger);
  }
}

static Camera::PixelFormat convert(AlviumConfigType::PixelMode mode)
{
  switch(mode) {
    convert_enum_case(Mono8);
    convert_enum_case(Mono10);
    convert_enum_case(Mono10p);
    convert_enum_case(Mono12);
    convert_enum_case(Mono12p);
    convert_enum_default(Format);
  }
}

static Camera::CorrectionType convert(AlviumConfigType::ImgCorrectionType corr_type)
{
  switch(corr_type) {
    convert_enum_case(DefectPixelCorrection);
    convert_enum_case(FixedPatternNoiseCorrection);
    convert_enum_default(Correction);
  }
}

static Camera::CorrectionSet convert(AlviumConfigType::ImgCorrectionSet corr_set)
{
  switch(corr_set) {
    convert_enum_case(Preset);
    convert_enum_case(User);
    convert_enum_default(Set);
  }
}

#undef convert_enum_case
#undef convert_enum_default

AlviumConfigCache::AlviumConfigCache(const Src& xtc, Camera& cam) :
  ConfigCache(xtc, cam)
{}

AlviumConfigCache::~AlviumConfigCache()
{}

bool AlviumConfigCache::_configure(bool apply)
{
  AlviumConfigType* cfg = reinterpret_cast<AlviumConfigType*>(_cur_config);
  if (cfg) {
    if (apply && !_cam.setAcquisitionMode(0)) {
      fprintf(stderr, "AlviumConfig: failed to apply acquisition mode configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply acquistion mode configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (apply && !_cam.setTriggerMode(convert(cfg->triggerMode()), cfg->exposureTime()*1e6)) {
      fprintf(stderr, "AlviumConfig: failed to apply trigger and exposure configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply trigger and exposure configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    switch(cfg->roiEnable()) {
      case AlviumConfigType::On:
        if (apply && !_cam.setImageRoi(cfg->offsetX(), cfg->offsetY(), cfg->width(), cfg->height())) {
          fprintf(stderr, "AlviumConfig: failed to apply ROI configuration.\n");
          snprintf(_error, MaxErrMsgLength,
                  "Alvium Config: failed to apply ROI configuration for %s !",
                  DetInfo::name(static_cast<const DetInfo&>(_config.src())));
          return false;
        }
        break;
      case AlviumConfigType::Centered:
        { uint32_t offset_x = (_cam.widthMax() - cfg->width()) / 2;
          uint32_t offset_y = (_cam.heightMax() - cfg->height()) / 2;
          Pds::VimbaConfig::setROI(*cfg, offset_x, offset_y, cfg->width(), cfg->height());
          if (apply && !_cam.setImageRoi(offset_x, offset_y, cfg->width(), cfg->height())) {
            fprintf(stderr, "AlviumConfig: failed to apply centered ROI configuration.\n");
            snprintf(_error, MaxErrMsgLength,
                     "Alvium Config: failed to apply centered ROI configuration for %s !",
                     DetInfo::name(static_cast<const DetInfo&>(_config.src())));
            return false;
          } else {
            Pds::VimbaConfig::setROI(*cfg, offset_x, offset_y, cfg->width(), cfg->height());
          }
        }
        break;
      case AlviumConfigType::Off:
        if (apply && !_cam.unsetImageRoi()) {
          fprintf(stderr, "AlviumConfig: failed to apply full frame configuration.\n");
          snprintf(_error, MaxErrMsgLength,
                  "Alvium Config: failed to apply full frame configuration for %s !",
                  DetInfo::name(static_cast<const DetInfo&>(_config.src())));
          return false;
        } else {
          Pds::VimbaConfig::setROI(*cfg, 0, 0, _cam.widthMax(), _cam.heightMax());
        }
        break;
      default:
        fprintf(stderr, "AlviumConfig: unsupported ROI mode.\n");
        snprintf(_error, MaxErrMsgLength,
                 "Alvium Config: unsupported ROI mode for %s !",
                 DetInfo::name(static_cast<const DetInfo&>(_config.src())));
        return false;
    }

    // put the sensor size information in the configuration
    Pds::VimbaConfig::setSensorInfo(*cfg, _cam.sensorWidth(), _cam.sensorHeight());

    if (apply && !_cam.setImageFlip(convert(cfg->reverseX()), convert(cfg->reverseY()))) {
      fprintf(stderr, "AlviumConfig: failed to apply image flip configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply image flip configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (apply && !_cam.setPixelFormat(convert(cfg->pixelMode()))) {
      fprintf(stderr, "AlviumConfig: failed to apply pixel bit depth configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply pixel bit depth configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (apply && !_cam.setAnalogControl(cfg->blackLevel(), cfg->gain(), cfg->gamma())) {
      fprintf(stderr, "AlviumConfig: failed to apply analog control configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply analog control configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (apply && !_cam.setContrastEnhancement(convert(cfg->contrastEnable()),
                                              cfg->contrastShape(),
                                              cfg->contrastDarkLimit(),
                                              cfg->contrastBrightLimit())) {
      fprintf(stderr, "AlviumConfig: failed to apply contrast enhancement configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply contrast enhancement configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (apply && !_cam.setImageCorrections(convert(cfg->correctionEnable()),
                                           convert(cfg->correctionType()),
                                           convert(cfg->correctionSet()))) {
      fprintf(stderr, "AlviumConfig: failed to apply image correction configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "Alvium Config: failed to apply image correction configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    // put the device information in the configuration
    Pds::VimbaConfig::setDeviceInfo(*cfg,
                                    _cam.deviceVendorName().c_str(),
                                    _cam.deviceFamilyName().c_str(),
                                    _cam.deviceModelName().c_str(),
                                    _cam.deviceManufacturer().c_str(),
                                    _cam.deviceVersion().c_str(),
                                    _cam.deviceSerialNumber().c_str(),
                                    _cam.deviceFirmwareID().c_str(),
                                    _cam.deviceFirmwareVersion().c_str());

    // show image format information
    printf("Image information:\n");
    printf("  Width (Max):            %lld (%lld)\n", _cam.width(), _cam.widthMax());
    printf("  Height (Max):           %lld (%lld)\n", _cam.height(), _cam.heightMax());
    printf("  Offset X:               %lld\n", _cam.offsetX());
    printf("  Offset Y:               %lld\n", _cam.offsetY());
    printf("  Reverse X:              %s\n", Bool::desc(_cam.reverseX()));
    printf("  Reverse Y:              %s\n", Bool::desc(_cam.reverseY()));
    printf("  Sensor Width:           %lld\n", _cam.sensorWidth());
    printf("  Sensor Height:          %lld\n", _cam.sensorHeight());
    printf("  Shutter Mode:           %s\n", _cam.shutterMode());
    printf("  Pixel Format:           %s\n", _cam.pixelFormat());
    printf("  Pixel Size:             %s\n", _cam.pixelSize());
    // show trigger information
    printf("Trigger information:\n");
    printf("  Trigger Mode:           %s\n", _cam.triggerMode());
    printf("  Exposure Mode:          %s\n", _cam.exposureMode());
    printf("  Exposure Time (us):     %f\n", _cam.exposureTime());
    // show contrast information
    printf("Contrast information:\n");
    printf("  Contrast Enable:        %s\n", Bool::desc(_cam.contrastEnable()));
    printf("  Contrast Shape:         %lld\n", _cam.contrastShape());
    printf("  Contrast Dark Limit:    %lld\n", _cam.contrastDarkLimit());
    printf("  Contrast Bright Limit:  %lld\n", _cam.contrastBrightLimit());
    // show analog control information
    printf("Analog control information:\n");
    printf("  Black Level:            %f\n", _cam.blackLevel());
    printf("  Black Selector:         %s\n", _cam.blackLevelSelector());
    printf("  Gain:                   %f\n", _cam.gain());
    printf("  Gain Selector:          %s\n", _cam.gainSelector());
    printf("  Gamma:                  %f\n", _cam.gamma());
    // show image correction information
    printf("Correction information:\n");
    printf("  Correction Mode:        %s\n", _cam.correctionMode());
    printf("  Correction Selector:    %s\n", _cam.correctionSelector());
    printf("  Correction Set:         %s\n", _cam.correctionSet());
    printf("  Correction Set Default: %s\n", _cam.correctionSetDefault());
    printf("  Correction Data Size:   %lld\n", _cam.correctionDataSize());
    printf("  Correction Entry Type:  %lld\n", _cam.correctionEntryType());
    // show device information
    printf("Device information:\n");
    printf("  Vendor name:            %s\n", _cam.deviceVendorName().c_str());
    printf("  Family name:            %s\n", _cam.deviceFamilyName().c_str());
    printf("  Model name:             %s\n", _cam.deviceModelName().c_str());
    printf("  Manufacturer ID:        %s\n", _cam.deviceManufacturer().c_str());
    printf("  Version:                %s\n", _cam.deviceVersion().c_str());
    printf("  Serial number           %s\n", _cam.deviceSerialNumber().c_str());
    printf("  Firmware ID:            %s\n", _cam.deviceFirmwareID().c_str());
    printf("  Firmware Version:       %s\n", _cam.deviceFirmwareVersion().c_str());
    printf("  User ID:                %s\n", _cam.deviceUserID().c_str());
    printf("  GenCP Version:          %lld.%lld\n", _cam.deviceGenCPMajorVersion(), _cam.deviceGenCPMinorVersion());
    printf("  TL Version:             %lld.%lld\n", _cam.deviceTLMajorVersion(), _cam.deviceTLMinorVersion());
    printf("  SFNC Version:           %lld.%lld.%lld\n",
           _cam.deviceSFNCMajorVersion(), _cam.deviceSFNCMinorVersion(), _cam.deviceSFNCPatchVersion());
    printf("  Link speed (B/s):       %lld\n", _cam.deviceLinkSpeed());
    printf("  Link timeout (us):      %f\n", _cam.deviceLinkCommandTimeout());
    printf("  Link limit mode:        %s\n", _cam.deviceLinkThroughputLimitMode());
    printf("  Link limit (B/s):       %lld\n", _cam.deviceLinkThroughputLimit());
    printf("  Scan type:              %s\n", _cam.deviceScanType());
    printf("  LED mode:               %s\n", _cam.deviceIndicatorMode());
    printf("  LED brightness:         %lld\n", _cam.deviceIndicatorLuminance());
    printf("  Temperature (C):        %f (%s)\n", _cam.deviceTemperature(), _cam.deviceTemperatureSelector());
    // show the maximum frame rate based on current _cam.ra settings
    printf("Maximum frame rate is %f Hz\n", _cam.acquisitionFrameRate());

    return true;
  } else {
    snprintf(_error, MaxErrMsgLength,
             "AlviumConfig: No configuration avaliable for %s",
             Pds::DetInfo::name(static_cast<const DetInfo&>(_config.src())));
    fprintf(stderr, "%s\n", _error);
    return false;
  }
}
