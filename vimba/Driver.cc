#include "Driver.hh"
#include "Errors.hh"
#include "Features.hh"

#include "pds/service/PathTools.hh"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <sstream>

using namespace Pds::Vimba;

VimbaException::VimbaException(VmbError_t code, const std::string& msg) :
  std::runtime_error(buildMsg(code, msg)),
  _code(code)
{}

VimbaException::~VimbaException() throw()
{}

VmbError_t VimbaException::code() const {
  return _code;
}

std::string VimbaException::buildMsg(VmbError_t code, const std::string& msg)
{
  return msg + ": " + ErrorCodes::desc(code);
}

Camera::Camera(VmbCameraInfo_t* info) :
  _cam(0),
  _num_features(0),
  _features(NULL),
  _capture(false)
{
  VmbError_t err = VmbErrorSuccess;

  if (info) {
    err = VmbCameraOpen(info->cameraIdString, VmbAccessModeFull, &_cam);
    if (err != VmbErrorSuccess) {
      fprintf(stderr,
              "Failure opening the camera (%s): %s\n",
              info->cameraIdString,
              ErrorCodes::desc(err));
    } else {
      err = VmbFeaturesList(_cam, NULL, 0, &_num_features, sizeof *_features);
      if (err != VmbErrorSuccess) {
        fprintf(stderr,
                "Failure determining the number of features of the camera (%s): %s\n",
                info->cameraIdString,
                ErrorCodes::desc(err));
      } else {
        _features = new VmbFeatureInfo_t[_num_features];
        err = VmbFeaturesList(_cam, _features, _num_features, &_num_features, sizeof *_features);
        if (err != VmbErrorSuccess) {
          fprintf(stderr,
                  "Failure to retrieve the features of the camera (%s): %s\n",
                  info->cameraIdString,
                  ErrorCodes::desc(err));
        }
      }
    }
    _info = *info;
  }
}

Camera::~Camera()
{
  close();
  if (_features) {
    delete[] _features;
  }
}

void Camera::close()
{
  if (isOpen()) {
    acquisitionStop();
    captureEnd();
    flushFrames();
    unregisterAllFrames();
    VmbCameraClose(_cam);
    _cam = 0;
  }
}

bool Camera::isOpen() const
{
  return _cam ? true : false;
}

bool Camera::isAcquiring() const
{
  try {
    return getBool(VMB_ACQ_STATUS) == VmbBoolTrue;
  } catch(VimbaException& e) {
    printf("Failed to get %s: %s\n", VMB_ACQ_STATUS, ErrorCodes::desc(e.code()));
    return false;
  }
}

bool Camera::imageCorrectionEnabled() const
{
  const char* mode = correctionMode();
  if (strcmp(mode, VMB_MODE_ON) == 0) {
    return true;
  } else if (strcmp(mode, VMB_MODE_OFF) == 0) {
    return false;
  } else {
    std::stringstream desc;
    desc << "Invalid correction mode: " << mode;
    throw VimbaException(VmbErrorOther, desc.str());
  }
}

VmbBool_t Camera::reverseX() const
{
  return getBool(VMB_REVERSE_X);
}

VmbBool_t Camera::reverseY() const
{
  return getBool(VMB_REVERSE_Y);
}

VmbBool_t Camera::contrastEnable() const
{
  return getBool(VMB_CONT_ENABLE);
}

VmbBool_t Camera::acquisitionFrameRateEnable() const
{
  return getBool(VMB_ACQ_FRAME_RATE_EN);
}

Camera::TriggerMode Camera::triggerModeEnum() const
{
  const char* trig_src = getEnum(VMB_TRIGGER_SOURCE);
  const char* trig_mode = getEnum(VMB_TRIGGER_MODE);

  if (strcmp(trig_src, "Software") == 0) {
    if (strcmp(trig_mode, VMB_MODE_OFF) == 0) {
      return FreeRun;
    } else if (strcmp(trig_mode, VMB_MODE_ON) == 0) {
      return Software;
    }
  } else if (strcmp(trig_src, "Line0") == 0) {
    if (strcmp(trig_mode, VMB_MODE_ON) == 0) {
      return External;
    }
  }

  return UnsupportedTrigger;
}

Camera::PixelFormat Camera::pixelFormatEnum() const
{
  const char* format = pixelFormat();
  if (strcmp(format, VMB_PIXEL_MONO8) == 0) {
    return Mono8;
  } else if (strcmp(format, VMB_PIXEL_MONO10) == 0) {
    return Mono10;
  } else if (strcmp(format, VMB_PIXEL_MONO10P) == 0) {
    return Mono10p;
  } else if (strcmp(format, VMB_PIXEL_MONO12) == 0) {
    return Mono12;
  } else if (strcmp(format, VMB_PIXEL_MONO12P) == 0) {
    return Mono12p;
  } else if (strcmp(format, VMB_PIXEL_MONO14) == 0) {
    return Mono14;
  } else if (strcmp(format, VMB_PIXEL_MONO16) == 0) {
    return Mono16;
  }

  return UnsupportedFormat;
}

VmbInt64_t Camera::payloadSize() const
{
  return getInt(VMB_PAYLOAD_SIZE);
}

VmbInt64_t Camera::deviceLinkThroughputLimit() const
{
  return getInt(VMB_DEV_LINK_TP_LIMIT);
}

VmbInt64_t Camera::deviceGenCPMajorVersion() const
{
  return getInt(VMB_DEV_GENCP_MAJ_VER);
}

VmbInt64_t Camera::deviceGenCPMinorVersion() const
{
  return getInt(VMB_DEV_GENCP_MIN_VER);
}

VmbInt64_t Camera::deviceTLMajorVersion() const
{
  return getInt(VMB_DEV_TL_MAJ_VER);
}

VmbInt64_t Camera::deviceTLMinorVersion() const
{
  return getInt(VMB_DEV_TL_MIN_VER);
}

VmbInt64_t Camera::deviceSFNCMajorVersion() const
{
  return getInt(VMB_DEV_SFNC_MAJ_VER);
}

VmbInt64_t Camera::deviceSFNCMinorVersion() const
{
  return getInt(VMB_DEV_SFNC_MIN_VER);
}

VmbInt64_t Camera::deviceSFNCPatchVersion() const
{
  return getInt(VMB_DEV_SFNC_PATCH_VER);
}

VmbInt64_t Camera::deviceLinkSpeed() const
{
  return getInt(VMB_DEV_LINK_SPEED);
}

VmbInt64_t Camera::deviceIndicatorLuminance() const
{
  return getInt(VMB_DEV_INDICATOR_LUM);
}

VmbInt64_t Camera::height() const
{
  return getInt(VMB_HEIGHT);
}

VmbInt64_t Camera::heightStep() const
{
  return getIntStep(VMB_HEIGHT);
}

VmbInt64_t Camera::heightMax() const
{
  return getInt(VMB_HEIGHT_MAX);
}

VmbInt64_t Camera::width() const
{
  return getInt(VMB_WIDTH);
}

VmbInt64_t Camera::widthStep() const
{
  return getIntStep(VMB_WIDTH);
}

VmbInt64_t Camera::widthMax() const
{
  return getInt(VMB_WIDTH_MAX);
}

VmbInt64_t Camera::offsetX() const
{
  return getInt(VMB_OFFSET_X);
}

VmbInt64_t Camera::offsetXStep() const
{
  return getIntStep(VMB_OFFSET_X);
}

VmbInt64_t Camera::offsetY() const
{
  return getInt(VMB_OFFSET_Y);
}

VmbInt64_t Camera::offsetYStep() const
{
  return getIntStep(VMB_OFFSET_Y);
}

VmbInt64_t Camera::sensorHeight() const
{
  return getInt(VMB_SENSOR_HEIGHT);
}

VmbInt64_t Camera::sensorWidth() const
{
  return getInt(VMB_SENSOR_WIDTH);
}

VmbInt64_t Camera::contrastShape() const
{
  return getInt(VMB_CONT_SHAPE);
}

VmbInt64_t Camera::contrastBrightLimit() const
{
  return getInt(VMB_CONT_BRIGHT_LIMIT);
}

VmbInt64_t Camera::contrastDarkLimit() const
{
  return getInt(VMB_CONT_DARK_LIMIT);
}

VmbInt64_t Camera::timestamp() const
{
  return getInt(VMB_TIMESTAMP_VALUE);
}

VmbInt64_t Camera::maxDriverBuffersCount() const
{
  return getInt(VMB_MAX_DRV_BUFFER_COUNT);
}

VmbInt64_t Camera::streamAnnounceBufferMinimum() const
{
  return getInt(VMB_STREAM_BUFFER_MIN);
}

VmbInt64_t Camera::streamAnnouncedBufferCount() const
{
  return getInt(VMB_STREAM_BUFFER_COUNT);
}

VmbInt64_t Camera::correctionDataSize() const
{
  return getInt(VMB_CORRECTION_DATA_SIZE);
}

VmbInt64_t Camera::correctionEntryType() const
{
  return getInt(VMB_CORRECTION_ENTRY_TYPE);
}

double Camera::acquisitionFrameRate() const
{
  return getDouble(VMB_ACQ_FRAME_RATE);
}

double Camera::deviceTemperature() const
{
  return getDouble(VMB_DEV_TEMPERATURE);
}

double Camera::deviceLinkCommandTimeout() const
{
  return getDouble(VMB_DEV_LINK_CMD_TMO);
}

double Camera::blackLevel() const
{
  return getDouble(VMB_BLACK_LEVEL);
}

double Camera::gain() const
{
  return getDouble(VMB_GAIN);
}

double Camera::gamma() const
{
  return getDouble(VMB_GAMMA);
}

double Camera::exposureTime() const
{
  return getDouble(VMB_EXPOSURE_TIME);
}

std::string Camera::deviceManufacturer() const
{
  return getString(VMB_DEV_MANUFACTURER);
}

std::string Camera::deviceVendorName() const
{
  return getString(VMB_DEV_VENDOR);
}

std::string Camera::deviceModelName() const
{
  return getString(VMB_DEV_MODEL);
}

std::string Camera::deviceFamilyName() const
{
  return getString(VMB_DEV_FAMILY_NAME);
}

std::string Camera::deviceFirmwareID() const
{
  return getString(VMB_DEV_FIRMWARE_ID);
}

std::string Camera::deviceFirmwareVersion() const
{
  return getString(VMB_DEV_FIRMWARE_VER);
}

std::string Camera::deviceVersion() const
{
  return getString(VMB_DEV_VERSION);
}

std::string Camera::deviceSerialNumber() const
{
  return getString(VMB_DEV_SERIAL_NUM);
}

std::string Camera::deviceUserID() const
{
  return getString(VMB_DEV_USER_ID);
}

const char* Camera::triggerMode() const
{
  switch (triggerModeEnum()) {
    case FreeRun:
      return "FreeRun";
    case External:
      return "External";
    case Software:
      return "Software";
    default:
      return "Unknown";
  }
}

const char* Camera::exposureMode() const
{
  return getEnum(VMB_EXPOSURE_MODE);
}

const char* Camera::deviceTemperatureSelector() const
{
  return getEnum(VMB_DEV_TEMP_SELECTOR);
}

const char* Camera::deviceScanType() const
{
  return getEnum(VMB_DEV_SCAN_TYPE);
}

const char* Camera::deviceLinkThroughputLimitMode() const
{
  return getEnum(VMB_DEV_LINK_TP_MODE);
}

const char* Camera::deviceIndicatorMode() const
{
  return getEnum(VMB_DEV_INDICATOR_MODE);
}

const char* Camera::shutterMode() const
{
  return getEnum(VMB_SHUTTER_MODE);
}

const char* Camera::pixelFormat() const
{
  return getEnum(VMB_PIXEL_FORMAT);
}

const char* Camera::pixelSize() const
{
  return getEnum(VMB_PIXEL_SIZE);
}

const char* Camera::contrastConfigurationMode() const
{
  return getEnum(VMB_CONT_CFG_MODE);
}

const char* Camera::blackLevelSelector() const
{
  return getEnum(VMB_BLACK_LEVEL_SEL);
}

const char* Camera::gainSelector() const
{
  return getEnum(VMB_GAIN_SELECTOR);
}

const char* Camera::acquisitionFrameRateMode() const
{
  return getEnum(VMB_ACQ_FRAME_RATE_MODE);
}

const char* Camera::streamBufferHandlingMode() const
{
  return getEnum(VMB_STREAM_BUFFER_MODE);
}

const char* Camera::correctionMode() const
{
  return getEnum(VMB_CORRECTION_MODE);
}

const char* Camera::correctionSelector() const
{
  return getEnum(VMB_CORRECTION_SELECTOR);
}

const char* Camera::correctionSet() const
{
  return getEnum(VMB_CORRECTION_SET);
}

const char* Camera::correctionSetDefault() const
{
  return getEnum(VMB_CORRECTION_SET_DEF);
}

bool Camera::checkImageRoi(VmbInt64_t offset_x, VmbInt64_t offset_y, VmbInt64_t width, VmbInt64_t height) const
{
  VmbInt64_t width_max = widthMax();
  VmbInt64_t width_step = widthStep();
  VmbInt64_t height_max = heightMax();
  VmbInt64_t height_step = heightStep();
  VmbInt64_t offset_x_step = offsetXStep();
  VmbInt64_t offset_y_step = offsetYStep();
  if ((width + offset_x) > width_max) {
    fprintf(stderr, "Requested width of the ROI (%lld) is larger than the maximum allowed width (%lld)\n",
            width + offset_x, width_max);
    return false;
  } else if ((height + offset_y) > height_max) {
    fprintf(stderr, "Requested height of the ROI (%lld) is larger than the maximum allowed height (%lld)\n",
            height + offset_y, height_max);
    return false;
  } else if (width % width_step != 0) {
    fprintf(stderr, "Requested width of the ROI (%lld) is not a multiple of the step size (%lld)\n",
            width, width_step);
    return false;
  } else if (height % height_step != 0) {
    fprintf(stderr, "Requested height of the ROI (%lld) is not a multiple of the step size (%lld)\n",
            height, height_step); 
    return false;
  } else if (offset_x % offset_x_step != 0) {
    fprintf(stderr, "Requested offsetX of the ROI (%lld) is not a multiple of the step size (%lld)\n",
            offset_x, offset_x_step);
    return false;
  } else if (offset_y % offset_y_step != 0) {
    fprintf(stderr, "Requested offsetY of the ROI (%lld) is not a multiple of the step size (%lld)\n",
            offset_y, offset_y_step);
    return false;
  } else {
    return true;
  }
}

bool Camera::unsetImageRoi()
{
  return setInt(VMB_OFFSET_X, 0) && setInt(VMB_OFFSET_Y, 0) &&
         setInt(VMB_WIDTH, widthMax()) && setInt(VMB_HEIGHT, heightMax());
}

bool Camera::setImageRoi(VmbInt64_t offset_x, VmbInt64_t offset_y, VmbInt64_t width, VmbInt64_t height)
{
  return checkImageRoi(offset_x, offset_y, width, height) && unsetImageRoi() &&
         setInt(VMB_WIDTH, width) && setInt(VMB_HEIGHT, height) &&
         setInt(VMB_OFFSET_X, offset_x) && setInt(VMB_OFFSET_Y, offset_y);
}

bool Camera::setImageFlip(VmbBool_t flipX, VmbBool_t flipY)
{
  return setBool(VMB_REVERSE_X, flipX) && setBool(VMB_REVERSE_Y, flipY);
}

bool Camera::setImageCorrections(VmbBool_t enabled, CorrectionType corr_type, CorrectionSet corr_set)
{
  return setCorrectionsEnabled(enabled) && setCorrectionsType(corr_type) && setCorrectionsSet(corr_set);
}

bool Camera::setCorrectionsEnabled(VmbBool_t enabled) {
  if (enabled) {
    return setEnum(VMB_CORRECTION_MODE, VMB_MODE_ON);
  } else {
    return setEnum(VMB_CORRECTION_MODE, VMB_MODE_OFF);
  }
}

bool Camera::setCorrectionsType(CorrectionType corr_type)
{
  switch (corr_type) {
  case DefectPixelCorrection:
    return setEnum(VMB_CORRECTION_SELECTOR, "DefectPixelCorrection");
  case FixedPatternNoiseCorrection:
    return setEnum(VMB_CORRECTION_SELECTOR, "FixedPatternNoiseCorrection");
  default:
    return false;
  }
}

bool Camera::setCorrectionsSet(CorrectionSet corr_set)
{
  switch (corr_set) {
  case Preset:
    return setEnum(VMB_CORRECTION_SET, "Preset");
  case User:
    return setEnum(VMB_CORRECTION_SET, "User");
  default:
    return false;
  }
}

bool Camera::setDeviceLinkThroughputLimit(VmbInt64_t limit)
{
  return setInt(VMB_DEV_LINK_TP_LIMIT, limit);
}

bool Camera::setAcquisitionMode(VmbUint32_t nframes)
{
  if (nframes == 0) {
    return setEnum(VMB_ACQ_MODE, "Continuous");
  } else if (nframes == 1) {
    return setEnum(VMB_ACQ_MODE, "SingleFrame");
  } else {
    return setEnum(VMB_ACQ_MODE, "MultiFrame") && setInt(VMB_ACQ_FRAME_COUNT, nframes);
  }
}

bool Camera::setTriggerMode(TriggerMode mode, double exposure_us, bool invert)
{
  const char* trig_src = NULL;
  const char* trig_mode = NULL;
  const char* trig_edge = NULL;

  switch(mode) {
  case FreeRun:
    trig_src  = "Software";
    trig_mode = VMB_MODE_OFF;
    break;
  case External:
    trig_src  = "Line0";
    trig_mode = VMB_MODE_ON;
    break;
  case Software:
    trig_src  = "Software";
    trig_mode = VMB_MODE_ON;
    break;
  default:
    fprintf(stderr, "Unsupported trigger mode: %d\n", mode);
    return false;
  }

  if (invert) {
    trig_edge = "FallingEdge";
  } else {
    trig_edge = "RisingEdge";
  }

  // set the trigger edge
  if (!setEnum(VMB_TRIGGER_ACTIVATION, trig_edge)) {
    return false;
  }

  // select the trigger i/o line
  if (!setEnum(VMB_LINE_SELECTOR, "Line0")) {
    return false;
  }

  // set the trigger line to input mode
  if (!setEnum(VMB_LINE_MODE, "Input")) {
    return false;
  }

  // set the trigger to frame start mode
  if (!setEnum(VMB_TRIGGER_SELECTOR, "FrameStart")) {
    return false;
  }

  // set the trigger source
  if (!setEnum(VMB_TRIGGER_SOURCE, trig_src)) {
    return false;
  }

  // set trigger mode
  if (!setEnum(VMB_TRIGGER_MODE, trig_mode)) {
    return false;
  }

  // set exposure
  if ((mode == External) && (exposure_us < 0.0)) {
    return setEnum(VMB_EXPOSURE_MODE, "TriggerWidth");
  } else {
    return setEnum(VMB_EXPOSURE_MODE, "Timed") && setDouble(VMB_EXPOSURE_TIME, exposure_us);
  }
}

bool Camera::setPixelFormat(PixelFormat format)
{
  switch(format) {
  case Mono8:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO8);
  case Mono10:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO10);
  case Mono10p:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO10P);
  case Mono12:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO12);
  case Mono12p:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO12P);
  case Mono14:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO14);
  case Mono16:
    return setEnum(VMB_PIXEL_FORMAT, VMB_PIXEL_MONO16);
  default:
    return false;
  }
}

bool Camera::setAnalogControl(double black_level, double gain, double gamma)
{
  return setDouble(VMB_BLACK_LEVEL, black_level) && setDouble(VMB_GAIN, gain) && setDouble(VMB_GAMMA, gamma);
}

bool Camera::setContrastEnhancement(VmbBool_t enabled,
                                    VmbInt64_t shape,
                                    VmbInt64_t dark_limit,
                                    VmbInt64_t bright_limit)
{
  return setBool(VMB_CONT_ENABLE, enabled) &&
         setInt(VMB_CONT_SHAPE, shape) &&
         setInt(VMB_CONT_DARK_LIMIT, dark_limit) &&
         setInt(VMB_CONT_BRIGHT_LIMIT, bright_limit);
}

void Camera::listFeatures() const
{
  VmbError_t err = VmbErrorSuccess;
  VmbBool_t bval = VmbBoolFalse;
  VmbUint32_t nsize = 0;
  char* sval = NULL;
  const char* eval = NULL;
  VmbInt64_t ival = 0, imin = 0, imax = 0, istep = 0;
  VmbBool_t bstep = VmbBoolFalse;
  double fval = 0.0, fmin = 0.0, fmax = 0.0, fstep = 0.0;

  printf("Listing features of camera (%s):\n", _info.cameraName);
  for (VmbUint32_t i=0; i<_num_features; i++) {
    printf("  %s:\n", _features[i].name ? _features[i].name : "");
    printf("    Display Name   - %s\n", _features[i].displayName ? _features[i].displayName : "");
    printf("    Tooltip        - %s\n", _features[i].tooltip ? _features[i].tooltip : "");
    printf("    Description    - %s\n", _features[i].description ? _features[i].description : "");
    printf("    SFNC Namespace - %s\n", _features[i].sfncNamespace ? _features[i].sfncNamespace : "");
    switch (_features[i].featureDataType) {
      case VmbFeatureDataBool:
        printf("    Type           - Bool\n");
        err = VmbFeatureBoolGet(_cam, _features[i].name, &bval);
        if (err == VmbErrorSuccess) {
          printf("    Value          - %d\n", bval);
        } else {
          printf("    Value          - error: %s\n", ErrorCodes::desc(err));
        }
        listAccess(_features[i].name);
        break;
      case VmbFeatureDataEnum:
        printf("    Type           - Enum\n");
        err = VmbFeatureEnumGet(_cam, _features[i].name, &eval);
        if (err == VmbErrorSuccess) {
          printf("    Value          - %s\n", eval);
        } else {
          printf("    Value          - error: %s\n", ErrorCodes::desc(err));
        }
        listEnumRange(_features[i].name);
        listAccess(_features[i].name);
        break;
      case VmbFeatureDataFloat:
        printf("    Type           - Float\n");
        err = VmbFeatureFloatGet(_cam, _features[i].name, &fval);
        if (err == VmbErrorSuccess) {
          printf("    Value          - %f\n", fval);
        } else {
          printf("    Value          - error: %s\n", ErrorCodes::desc(err));
        }
        err = VmbFeatureFloatRangeQuery(_cam, _features[i].name, &fmin, &fmax);
        if (err == VmbErrorSuccess) {
          printf("    Range          - %f to %f\n", fmin, fmax);
        } else {
          printf("    Range          - error: %s\n", ErrorCodes::desc(err));
        }
        err = VmbFeatureFloatIncrementQuery(_cam, _features[i].name, &bstep, &fstep);
        if (err == VmbErrorSuccess) {
          if (bstep) {
            printf("    Step           - %f\n", fstep);
          } else {
            printf("    Step           - None\n");
          }
        } else {
          printf("    Step           - error: %s\n", ErrorCodes::desc(err));
        }
        listAccess(_features[i].name);
        break;
      case VmbFeatureDataInt:
        printf("    Type           - Integer\n");
        err = VmbFeatureIntGet(_cam, _features[i].name, &ival);
        if (err == VmbErrorSuccess) {
          printf("    Value          - %lld\n", ival);
        } else {
          printf("    Value          - error: %s\n", ErrorCodes::desc(err));
        }
        err = VmbFeatureIntRangeQuery(_cam, _features[i].name, &imin, &imax);
        if (err == VmbErrorSuccess) {
          printf("    Range          - %lld to %lld\n", imin, imax);
        } else {
          printf("    Range          - error: %s\n", ErrorCodes::desc(err));
        }
        err = VmbFeatureIntIncrementQuery(_cam, _features[i].name, &istep);
        if (err == VmbErrorSuccess) {
          printf("    Step           - %lld\n", istep);
        } else {
          printf("    Step           - error: %s\n", ErrorCodes::desc(err));
        }
        listAccess(_features[i].name);
        break;
      case VmbFeatureDataString:
        printf("    Type           - String\n");
        err = VmbFeatureStringGet(_cam, _features[i].name, NULL, 0, &nsize);
        if (err == VmbErrorSuccess) {
          sval = new char[nsize];
          err = VmbFeatureStringGet(_cam, _features[i].name, sval, nsize, &nsize);
          if (err == VmbErrorSuccess) {
            printf("    Value          - %s\n", sval);
          } else {
            printf("    Value          - error: %s\n", ErrorCodes::desc(err));
          }
          delete[] sval;
          sval = 0;
        } else {
          printf("    Value          - error: %s\n", ErrorCodes::desc(err));
        }
        err = VmbFeatureStringMaxlengthQuery(_cam, _features[i].name, &nsize);
        if (err == VmbErrorSuccess) {
          printf("    Range          - %u chars\n", nsize);
        } else {
          printf("    Range          - error: %s\n", ErrorCodes::desc(err));
        }
        listAccess(_features[i].name);
        break;
      case VmbFeatureDataCommand:
        printf("    Type           - Command\n");
        break;
      case VmbFeatureDataRaw:
        printf("    Type           - Raw\n");
        break;
      default:
        printf("    Type           - Unknown\n");
        printf("    Value          - None\n");
        break;
    }
  }
}

void Camera::listAccess(const char* name) const
{
  VmbError_t err = VmbErrorSuccess;
  VmbBool_t readable = VmbBoolFalse;
  VmbBool_t writeable = VmbBoolFalse;

  err = VmbFeatureAccessQuery(_cam, name, &readable, &writeable);
  if (err == VmbErrorSuccess) {
    printf("    isReadable     - %s\n", Bool::desc(readable));
    printf("    isWriteable    - %s\n", Bool::desc(writeable));
  } else {
    printf("    isReadable     - error: %s\n", ErrorCodes::desc(err));
    printf("    isWriteable    - error: %s\n", ErrorCodes::desc(err));
  }
}

void Camera::listEnumRange(const char* name) const
{
  VmbError_t err = VmbErrorSuccess;
  VmbUint32_t nenum = 0;
  const char** enums = NULL;

  err = VmbFeatureEnumRangeQuery(_cam, name, NULL, 0, &nenum);
  if (err == VmbErrorSuccess) {
    enums = new const char*[nenum];
    err = VmbFeatureEnumRangeQuery(_cam, name, enums, nenum, &nenum);
    if (err == VmbErrorSuccess) {
      printf("    Range          -");
      for (VmbUint32_t n=0; n<nenum; n++) {
        if (n==0)
          printf(" %s", enums[n]);
        else
          printf(", %s", enums[n]);
      }
      printf("\n");
    } else {
      fprintf(stderr, "Failed to Query enum values for feature %s: %s\n", name,  ErrorCodes::desc(err));
    }
    delete[] enums;
    enums = NULL;
  } else {
    fprintf(stderr, "Failed to Query number of enums for feature %s: %s\n", name,  ErrorCodes::desc(err));
  }
}

std::string Camera::getGeniCamPath()
{
  std::string path = Pds::PathTools::getBuildPathStr();
  path.append("/vimba/VimbaUSBTL/CTI/x86_64bit");
  return path;
}

bool Camera::getVersionInfo(VmbVersionInfo_t* info)
{
  if (info) {
    VmbError_t err = VmbVersionQuery(info, sizeof(*info));
    if (err != VmbErrorSuccess) {
      fprintf(stderr, "VmbVersionQuery failed: %s\n", ErrorCodes::desc(err));
      return false;
    } else {
      return true;
    }
  } else {
    fprintf(stderr, "No memory allocated for VmbVersionInfo!\n");
    return false;
  }
}

bool Camera::getCameraInfo(VmbCameraInfo_t* info, VmbUint32_t index, const char* serial_id)
{
  VmbError_t  err = VmbErrorSuccess;
  VmbUint32_t ncount = 0;
  VmbUint32_t ninfo = 0;
  VmbCameraInfo_t* cameras = NULL;

  err = VmbCamerasList(NULL, 0, &ncount, sizeof *cameras);
  if (err == VmbErrorSuccess) {
    cameras = new VmbCameraInfo_t[ncount];
    err = VmbCamerasList(cameras, ncount, &ninfo, sizeof *cameras);
    if (err == VmbErrorSuccess || err == VmbErrorMoreData) {
      if (ninfo < ncount)
        ncount = ninfo;
      if (serial_id) {
        for (VmbUint32_t n=0; n<ncount; n++) {
          if (strcmp(serial_id, cameras[n].serialString) == 0) {
            memcpy(info, &cameras[n], sizeof *cameras);
            return true;
          }
        }
      } else if(index < ncount) {
        memcpy(info, &cameras[index], sizeof *cameras);
        return true;
      }
    } else {
      fprintf(stderr, "Failed to retrieve camera info: %s\n", ErrorCodes::desc(err));
    }
    delete[] cameras;
  } else {
    fprintf(stderr, "Failed to retrieve number of available cameras: %s\n", ErrorCodes::desc(err));
  }

  return false;
}

bool Camera::registerFrame(const VmbFrame_t* frame)
{
  VmbError_t err = VmbFrameAnnounce(_cam, frame, sizeof *frame);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to register frame (%p): %s\n", (void*) frame, ErrorCodes::desc(err));
    return false;
  } else { 
    return true;
  }
}

bool Camera::registerFrames(const VmbFrame_t* frame, VmbUint32_t nframes)
{
  for (VmbUint32_t n=0; n<nframes; n++) {
    if (!registerFrame(frame + n)) {
      return false;
    }
  }

  return true;
}

bool Camera::unregisterFrame(const VmbFrame_t* frame)
{
  VmbError_t err = VmbFrameRevoke(_cam, frame);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to unregister frame (%p): %s\n", (void*) frame, ErrorCodes::desc(err));
    return false;
  } else { 
    return true;
  }
}

bool Camera::unregisterFrames(const VmbFrame_t* frame, VmbUint32_t nframes)
{
  for (VmbUint32_t n=0; n<nframes; n++) {
    if (!unregisterFrame(frame + n)) {
      return false;
    }
  }

  return true;
}

bool Camera::unregisterAllFrames()
{
  VmbError_t err = VmbFrameRevokeAll(_cam);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to unregister all frames: %s\n", ErrorCodes::desc(err));
    return false;
  } else { 
    return true;
  }
}

bool Camera::captureStart()
{
  if (_capture)
    return true;

  VmbError_t err = VmbCaptureStart(_cam);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to start capture: %s\n", ErrorCodes::desc(err));
    return false;
  } else {
    _capture = true;
    return true;
  }
}

bool Camera::captureEnd()
{
  if (!_capture)
    return true;

  VmbError_t err = VmbCaptureEnd(_cam);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to end capture: %s\n", ErrorCodes::desc(err));
    return false;
  } else {
    _capture = false;
    return true;
  }
}

bool Camera::acquisitionStart()
{
  return runCommand(VMB_ACQ_START);
}

bool Camera::acquisitionStop()
{
  return runCommand(VMB_ACQ_STOP);
}

bool Camera::queueFrame(const VmbFrame_t* frame, VmbFrameCallback cb)
{
  VmbError_t err = VmbCaptureFrameQueue(_cam, frame, cb);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to queue frame (%p): %s\n", (void*) frame, ErrorCodes::desc(err));
    return false;
  } else {
    return true;
  }
}

bool Camera::queueFrames(const VmbFrame_t* frame, VmbUint32_t nframes, VmbFrameCallback cb)
{
  for (VmbUint32_t n=0; n<nframes; n++) {
    if (!queueFrame(frame + n, cb)) {
      return false;
    }
  }

  return true;
}

bool Camera::waitFrame(const VmbFrame_t* frame, VmbUint32_t timeout, bool* had_timeout)
{
  VmbError_t err = VmbCaptureFrameWait(_cam, frame, timeout);
  if (err != VmbErrorSuccess && err != VmbErrorTimeout) {
    fprintf(stderr, "Failed wait on frame (%p): %s\n", (void*) frame, ErrorCodes::desc(err));
    return false;
  } else {
    if (had_timeout)
      *had_timeout = (err == VmbErrorTimeout);
    return true;
  }
}

bool Camera::flushFrames()
{
  VmbError_t err = VmbCaptureQueueFlush(_cam);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to flush capure queue: %s\n", ErrorCodes::desc(err));
    return false;
  } else {
    return true;
  }  
}

bool Camera::reset()
{
  return runCommand(VMB_DEV_RESET);
}

bool Camera::resetTimestamp()
{
  return runCommand(VMB_TIMESTAMP_RESET);
}

bool Camera::latchTimestamp()
{
  return runCommand(VMB_TIMESTAMP_LATCH);
}

bool Camera::sendSoftwareTrigger()
{
  return runCommand(VMB_TRIGGER_SOFTWARE);
}

const char* Camera::getEnum(const char* name) const
{
  const char* value = NULL;
  VmbError_t err = getEnumFeature(name, &value);
  if (err == VmbErrorSuccess) {
    return value;
  } else {
    std::stringstream desc;
    desc << "failed to get " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

VmbBool_t Camera::getBool(const char* name) const
{
  VmbBool_t value = VmbBoolFalse;
  VmbError_t err = getBoolFeature(name, &value);
  if (err == VmbErrorSuccess) {
    return value;
  } else {
    std::stringstream desc;
    desc << "failed to get " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

VmbInt64_t Camera::getInt(const char* name) const
{
  VmbInt64_t value = 0;
  VmbError_t err = getIntFeature(name, &value);
  if (err == VmbErrorSuccess) {
    return value;
  } else {
    std::stringstream desc;
    desc << "failed to get " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

VmbInt64_t Camera::getIntStep(const char* name) const
{
  VmbInt64_t step = 0;
  VmbError_t err = VmbFeatureIntIncrementQuery(_cam, name, &step);
  if (err == VmbErrorSuccess) {
    return step;
  } else {
    std::stringstream desc;
    desc << "failed to get increment of " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

double Camera::getDouble(const char* name) const
{
  double value = 0.0;
  VmbError_t err = getFloatFeature(name, &value);
  if (err == VmbErrorSuccess) {
    return value;
  } else {
    std::stringstream desc;
    desc << "failed to get " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

std::string Camera::getString(const char* name) const
{
  std::string value;
  VmbError_t err = getStringFeature(name, value);
  if (err == VmbErrorSuccess) {
    return value;
  } else {
    std::stringstream desc;
    desc << "failed to get " << name << " feature";
    throw VimbaException(err, desc.str());
  }
}

bool Camera::setEnum(const char* name, const char* value)
{
  VmbError_t err = VmbErrorSuccess;

  err = setEnumFeature(name, value);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to set %s to enum value '%s': %s\n", name, value, ErrorCodes::desc(err));
  }

  return err == VmbErrorSuccess;
}

bool Camera::setBool(const char* name, VmbBool_t value)
{
  VmbError_t err = VmbErrorSuccess;

  err = setBoolFeature(name, value);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to set %s to bool value '%s': %s\n",
            name, Bool::desc(value), ErrorCodes::desc(err));
  }

  return err == VmbErrorSuccess;
}

bool Camera::setInt(const char* name, VmbInt64_t value)
{
  VmbError_t err = VmbErrorSuccess;

  err = setIntFeature(name, value);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to set %s to integer value %lld: %s\n", name, value, ErrorCodes::desc(err));
  }

  return err == VmbErrorSuccess;
}

bool Camera::setDouble(const char* name, double value)
{
  VmbError_t err = VmbErrorSuccess;
  err = setFloatFeature(name, value);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "Failed to set %s to float value %f: %s\n", name, value, ErrorCodes::desc(err));
  }

  return err == VmbErrorSuccess;
}

bool Camera::runCommand(const char* name)
{
  VmbError_t err = VmbErrorSuccess;
    
  err = VmbFeatureCommandRun(_cam, name);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s command failed: %s\n", name, ErrorCodes::desc(err));
  }

  return err == VmbErrorSuccess;return err == VmbErrorSuccess;
}

VmbError_t Camera::getBoolFeature(const char* name, VmbBool_t* value) const
{
  return VmbFeatureBoolGet(_cam, name, value);
}

VmbError_t Camera::setBoolFeature(const char* name, VmbBool_t value)
{
  return VmbFeatureBoolSet(_cam, name, value);
}

VmbError_t Camera::getEnumFeature(const char* name, const char** value) const
{
  return VmbFeatureEnumGet(_cam, name, value);
}

VmbError_t Camera::setEnumFeature(const char* name, const char* value)
{
  return VmbFeatureEnumSet(_cam, name, value);
}

VmbError_t Camera::getIntFeature(const char* name, VmbInt64_t* value) const
{
  return VmbFeatureIntGet(_cam, name, value);
}

VmbError_t Camera::setIntFeature(const char* name, VmbInt64_t value)
{
  return VmbFeatureIntSet(_cam, name, value);
}

VmbError_t Camera::getFloatFeature(const char* name, double* value) const
{
  return VmbFeatureFloatGet(_cam, name, value);
}

VmbError_t Camera::setFloatFeature(const char* name, double value)
{
  return VmbFeatureFloatSet(_cam, name, value);
}

VmbError_t Camera::getStringFeature(const char* name, std::string& value) const
{
  VmbError_t err = VmbErrorSuccess;
  VmbUint32_t size = 0;
  char* buffer = NULL;
  err = VmbFeatureStringGet(_cam, name, NULL, 0, &size);
  if (err == VmbErrorSuccess) {
    buffer = new char[size];
    err = VmbFeatureStringGet(_cam, name, buffer, size, NULL);
    if (err == VmbErrorSuccess) {
      value.assign(buffer); 
    }
    delete[] buffer;
  }

  return err;
}

VmbError_t Camera::setStringFeature(const char* name, std::string value)
{
  return VmbFeatureStringSet(_cam, name, value.c_str());
}

VmbError_t Camera::getStringFeature(const char* name, char* value, VmbUint32_t size) const
{
  return VmbFeatureStringGet(_cam, name, value, size, NULL);
}

VmbError_t Camera::setStringFeature(const char* name, const char* value)
{
  return VmbFeatureStringSet(_cam, name, value);
}


