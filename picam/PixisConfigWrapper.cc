#include "pds/picam/PixisConfigWrapper.hh"

#include <stdio.h>
#include <stdlib.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/config/CfgClientNfs.hh"

namespace Pds
{

PixisConfigWrapper::PixisConfigWrapper(CfgClientNfs& cfg) :
  _cfg(cfg),
  _cfgtc(_pixisConfigType, cfg.src()),
  _config()
{}

PixisConfigWrapper::~PixisConfigWrapper()
{}

PixisConfigType& PixisConfigWrapper::config()
{
  return _config;
}

int PixisConfigWrapper::fetch(Transition* tr)
{
  int iConfigCameraFail = 0;
  int iConfigSize = _cfg.fetch(*tr, _pixisConfigType, &_config, sizeof(_config));

  if ( iConfigSize == 0 ) // no config data is found in the database.
  {
    printf( "PixisConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
    _config = PixisConfigType(
      2048,   // Width
      2048,   // Height
      0,      // OrgX
      0,      // OrgX
      1,      // BinX
      1,      // BinX
      0.1,    // Exposure time
      25.0f,  // Cooling temperature
      2,      // Readout speed (MHz)
      PixisConfigType::Medium,    // gain mode
      PixisConfigType::LowNoise,  // adc mode
      PixisConfigType::Software,  // trigger mode
      2048,   // Active Width
      2048,   // Active Height
      2,      // Top Margin
      3,      // Bottom Margin
      50,     // Left Margin
      50,     // Right Margin
      1,      // Clean Cycle Count
      2048,   // Clean Cycle Height
      2,      // Clean Final Height
      512,    // Clean Final Height Count
      0,      // Masked Height
      0,      // Kinetic Height
      30.2f,  // Vertical Shift Speed
      1,      // Info report interval
      1,      // Redout event code
      1       // number integration shots
    );
  }

  if ( iConfigSize != 0 && iConfigSize != sizeof(_config) )
  {
    printf( "PixisConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
    iConfigSize, (int) sizeof(_config) );

    _config       = PixisConfigType();
    iConfigCameraFail  = 1;
  }

  return iConfigCameraFail;
}

void PixisConfigWrapper::insert(InDatagram* in)
{
  static bool bConfigAllocated = false;
  if ( !bConfigAllocated )
  {
    // insert assumes we have enough space in the memory pool for in datagram
    _cfgtc.alloc( sizeof(_config) );
    bConfigAllocated = true;
  }

  in->insert(_cfgtc, &_config);
}

uint32_t PixisConfigWrapper::width() const
{
  return _config.width();
}

uint32_t PixisConfigWrapper::height() const
{
  return _config.height();
}

uint32_t PixisConfigWrapper::orgX() const
{
  return _config.orgX();
}

uint32_t PixisConfigWrapper::orgY() const
{
  return _config.orgY();
}

uint32_t PixisConfigWrapper::binX() const
{
  return _config.binX();
}

uint32_t PixisConfigWrapper::binY() const
{
  return _config.binY();
}

float PixisConfigWrapper::exposureTime() const
{
  return _config.exposureTime();
}

float PixisConfigWrapper::coolingTemp() const
{
  return _config.coolingTemp();
}

float PixisConfigWrapper::readoutSpeed() const
{
  return _config.readoutSpeed();
}

uint16_t PixisConfigWrapper::gainIndex() const
{
  return _config.gainMode();
}

int16_t PixisConfigWrapper::infoReportInterval() const
{
  return _config.infoReportInterval();
}

uint16_t PixisConfigWrapper::exposureEventCode() const
{
  return _config.exposureEventCode();
}

uint32_t PixisConfigWrapper::numIntegrationShots() const
{
  return _config.numIntegrationShots();
}

void PixisConfigWrapper::setNumIntegrationShots(unsigned n)
{
  Pds::PixisConfig::setNumIntegrationShots(_config,n);
}

void PixisConfigWrapper::setSize(unsigned w, unsigned h)
{
  Pds::PixisConfig::setSize(_config, w, h);
}

uint32_t PixisConfigWrapper::frameSize() const
{
  return _config.frameSize();
}

uint32_t PixisConfigWrapper::numPixelsX() const
{
  return _config.numPixelsX();
}

uint32_t PixisConfigWrapper::numPixelsY() const
{
  return _config.numPixelsY();
}

uint32_t PixisConfigWrapper::numPixels() const
{
  return _config.numPixels();
}

uint16_t PixisConfigWrapper::adcMode() const
{
  return _config.adcMode();
}

uint16_t PixisConfigWrapper::triggerMode() const
{
  return _config.triggerMode();
}

double PixisConfigWrapper::vsSpeed() const
{
  return _config.vsSpeed();
}

} //namespace Pds
