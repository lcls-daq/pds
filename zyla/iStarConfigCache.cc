#include "iStarConfigCache.hh"
#include "Driver.hh"

#include "pdsdata/xtc/DetInfo.hh"

#define convert_enum_case(name) \
  case iStarConfigType::name:    \
    return Driver::name;

#define convert_enum_default(name) \
  default:                         \
    return Driver::Unsupported##name;

using namespace Pds::Zyla;

static Driver::ReadoutRate convert(iStarConfigType::ReadoutRate rate)
{
  switch(rate) {
    convert_enum_case(Rate280MHz);
    convert_enum_case(Rate100MHz);
    convert_enum_default(Readout);
  }
}

static Driver::GainMode convert(iStarConfigType::GainMode gain)
{
  switch(gain) {
    convert_enum_case(HighWellCap12Bit);
    convert_enum_case(LowNoise12Bit);
    convert_enum_case(LowNoiseHighWellCap16Bit);
    convert_enum_default(Gain);
  }
}

static Driver::TriggerMode convert(iStarConfigType::TriggerMode trigger)
{
  switch(trigger) {
    convert_enum_case(Internal);
    convert_enum_case(ExternalLevelTransition);
    convert_enum_case(ExternalStart);
    convert_enum_case(ExternalExposure);
    convert_enum_case(Software);
    convert_enum_case(Advanced);
    convert_enum_case(External);
    convert_enum_default(Trigger);
  }
}

static Driver::FanSpeed convert(iStarConfigType::FanSpeed speed)
{
  switch(speed) {
    convert_enum_case(Off);
    convert_enum_case(On);
    convert_enum_default(FanSpeed);
  }
}

static Driver::GateMode convert(iStarConfigType::GateMode gate)
{
  switch(gate) {
    convert_enum_case(CWOn);
    convert_enum_case(CWOff);
    convert_enum_case(FireOnly);
    convert_enum_case(GateOnly);
    convert_enum_case(FireAndGate);
    convert_enum_case(DDG);
    convert_enum_default(Gate);
  }
}

static Driver::InsertionDelay convert(iStarConfigType::InsertionDelay delay)
{
  switch(delay) {
    convert_enum_case(Normal);
    convert_enum_case(Fast);
    convert_enum_default(Delay);
  }
}

#undef convert_enum_case
#undef convert_enum_default

iStarConfigCache::iStarConfigCache(const Src& xtc, Driver& driver) :
  ConfigCache(xtc, driver),
  _cfg(NULL)
{}

iStarConfigCache::~iStarConfigCache()
{}

bool iStarConfigCache::_configure(bool apply)
{
  _cfg = reinterpret_cast<const iStarConfigType*>(current());
  if (_cfg) {
    if (!_driver.set_image(_cfg->width(),
                           _cfg->height(),
                           _cfg->orgX(),
                           _cfg->orgY(),
                           _cfg->binX(),
                           _cfg->binY(),
                           _cfg->noiseFilter(),
                           _cfg->blemishCorrection())) {
      fprintf(stderr, "iStarConfig: failed to apply image/ROI configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply image/ROI configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (!_driver.set_intensifier(convert(_cfg->gateMode()),
                                 convert(_cfg->insertionDelay()),
                                 _cfg->mcpGain(),
                                 _cfg->mcpIntelligate())) {
      fprintf(stderr, "iStarConfig: failed to apply intensifier configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply intensifier configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    if (!_driver.set_readout(Driver::Global,
                             convert(_cfg->readoutRate()),
                             convert(_cfg->gainMode()))) {
      fprintf(stderr, "iStarConfig: failed to apply readout configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply readout configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));
      return false;
    }

    // Test if a supported trigger mode for the camera has been chosen
    if (_cfg->triggerMode() != iStarConfigType::ExternalExposure && 
        _cfg->triggerMode() != iStarConfigType::External) {

      fprintf(stderr, "iStarConfig: unsupported trigger configuration mode: %d.\n",
              _cfg->triggerMode());
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: unsupported trigger configuration mode for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));

      return false;
    }

    if (!_driver.set_trigger(convert(_cfg->triggerMode()),
                             _cfg->triggerDelay(),
                             _cfg->overlap())) {
      fprintf(stderr, "iStarConfig: failed to apply trigger configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply trigger configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));

      return false;
    }

    if (!_driver.set_exposure(_cfg->exposureTime())) {
      fprintf(stderr, "iStarConfig: failed to apply exposure time configuration.\n");
      fprintf(stderr, "iStarConfig: Requested exposure of %.5f s is outside the "
              "allowed range of %.5f to %.5f s.\n",
              _cfg->exposureTime(), _driver.exposure_min(), _driver.exposure_max());
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failure setting exposure time for %s !\n"
               "\nRequested exposure of %.5f s is outside the range of %.5f "
               "to %.5f s allowed by the current camera settings.",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())),
               _cfg->exposureTime(),
               _driver.exposure_min(),
               _driver.exposure_max());
      if (_driver.overlap_mode()) {
        size_t len = strlen(_error);
        if (len < MaxErrMsgLength) {
          snprintf(_error + len, MaxErrMsgLength - len,
                   "\n\nCamera is in overlap readout mode!\n"
                   "Exposure time must be greater than the frame readout time "
                   "of %.5f s + a bit extra in this mode.",
                   _driver.readout_time());
        }
      }

      return false;
    }

    if (!_driver.set_cooling(_cfg->cooling(),
                             Driver::Temp_0C,
                             convert(_cfg->fanSpeed()))) {
      fprintf(stderr, "iStarConfig: failed to apply cooling configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply cooling configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));

      return false;
    }

    if (!_driver.configure()) {
      fprintf(stderr, "iStarConfig: failed to apply configuration.\n");
      snprintf(_error, MaxErrMsgLength,
               "iStar Config: failed to apply configuration for %s !",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())));

      return false;
    }

    // Print cooling status info
    if (_driver.cooling_on()) {
      _driver.get_cooling_status(_wc_buffer, MaxAtMsgLength);
      printf("Current cooling status (temp): %ls (%.2f C)\n",
             _wc_buffer, _driver.temperature());
    }

    // Print other configure info
    printf("Image ROI (w,h)       : %lld, %lld\n",
           _driver.image_width(), _driver.image_height());
    printf("          (orgX,orgY) : %lld, %lld\n",
           _driver.image_orgX(), _driver.image_orgY());
    printf("          (binX,binY) : %lld, %lld\n",
           _driver.image_binX(), _driver.image_binY());
    printf("Image exposure time (sec) : %g\n", _driver.exposure());

    _driver.get_shutter_mode(_wc_buffer, MaxAtMsgLength);
    printf("Shutter mode: %ls\n", _wc_buffer);

    _driver.get_trigger_mode(_wc_buffer, MaxAtMsgLength);
    printf("Trigger mode: %ls\n", _wc_buffer);

    _driver.get_gain_mode(_wc_buffer, MaxAtMsgLength);
    printf("Gain mode: %ls\n", _wc_buffer);

    _driver.get_gate_mode(_wc_buffer, MaxAtMsgLength);
    printf("Gate mode: %ls\n", _wc_buffer);

    _driver.get_insertion_delay(_wc_buffer, MaxAtMsgLength);
    printf("Insertion delay: %ls\n", _wc_buffer);

    if (_driver.mcp_intelligate_on()) {
      printf("MCP Intelligate is enabled!\n");
    }

    printf("MCP gain: %lld\n", _driver.mcp_gain());

    _driver.get_readout_rate(_wc_buffer, MaxAtMsgLength);
    printf("Pixel readout rate: %ls\n", _wc_buffer);
    if (_driver.overlap_mode()) {
      printf("Camera readout set to overlap mode!\n");
    }
    printf("Estimated readout time for the camera (sec): %g\n",
           _driver.readout_time());

    return true;
  } else {
    snprintf(_error, MaxErrMsgLength,
             "iStarConfig: No configuration avaliable for %s",
             Pds::DetInfo::name(static_cast<const DetInfo&>(_config.src())));
    fprintf(stderr, "%s\n", _error);
    return false;
  }
}
