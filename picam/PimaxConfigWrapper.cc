#include "pds/picam/PimaxConfigWrapper.hh"

#include <stdio.h>
#include <stdlib.h>

#include "pds/xtc/CDatagram.hh"
#include "pds/config/CfgClientNfs.hh"

namespace Pds
{

PimaxConfigWrapper::PimaxConfigWrapper(CfgClientNfs& cfg) :
  _cfg(cfg),
  _cfgtc(_pimaxConfigType, cfg.src()),
  _config()
{}

PimaxConfigWrapper::~PimaxConfigWrapper()
{}

PimaxConfigType& PimaxConfigWrapper::config()
{
  return _config;
}

int PimaxConfigWrapper::fetch(Transition* tr)
{
  int iConfigCameraFail = 0;
  int iConfigSize = _cfg.fetch(*tr, _pimaxConfigType, &_config, sizeof(_config));

  if ( iConfigSize == 0 ) // no config data is found in the database.
  {
    printf( "PimaxConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
    _config = PimaxConfigType(
      16,     // Width
      16,     // Height
      0,      // OrgX
      0,      // OrgX
      1,      // BinX
      1,      // BinX
      0.001,  // Exposure time
      25.0f,  // Cooling temperature
      2,      // Readout speed (MHz)
      1,      // gain index
      1,      // Intensifier Gain
      250.0,  // Gate delay (ns)
      1.0e6,  // Gate Width (ns)
      0,      // Masked Height
      0,      // Kinetic Height
      0.0f,   // Vertical Shift Speed
      1,      // Info report interval
      1,      // Redout event code
      1       // number integration shots
    );
  }

  if ( iConfigSize != 0 && iConfigSize != sizeof(_config) )
  {
    printf( "PimaxConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
    iConfigSize, (int) sizeof(_config) );

    _config       = PimaxConfigType();
    iConfigCameraFail  = 1;
  }

  return iConfigCameraFail;
}

void PimaxConfigWrapper::insert(InDatagram* in)
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

uint32_t PimaxConfigWrapper::width() const
{
  return _config.width();
}

uint32_t PimaxConfigWrapper::height() const
{
  return _config.height();
}

uint32_t PimaxConfigWrapper::orgX() const
{
  return _config.orgX();
}

uint32_t PimaxConfigWrapper::orgY() const
{
  return _config.orgY();
}

uint32_t PimaxConfigWrapper::binX() const
{
  return _config.binX();
}

uint32_t PimaxConfigWrapper::binY() const
{
  return _config.binY();
}

float PimaxConfigWrapper::exposureTime() const
{
  return _config.exposureTime();
}

float PimaxConfigWrapper::coolingTemp() const
{
  return _config.coolingTemp();
}

float PimaxConfigWrapper::readoutSpeed() const
{
  return _config.readoutSpeed();
}

uint16_t PimaxConfigWrapper::gainIndex() const
{
  return _config.gainIndex();
}

int16_t PimaxConfigWrapper::infoReportInterval() const
{
  return _config.infoReportInterval();
}

uint16_t PimaxConfigWrapper::exposureEventCode() const
{
  return _config.exposureEventCode();
}

uint32_t PimaxConfigWrapper::numIntegrationShots() const
{
  return _config.numIntegrationShots();
}

void PimaxConfigWrapper::setNumIntegrationShots(unsigned n)
{
  Pds::PimaxConfig::setNumIntegrationShots(_config,n);
}

void PimaxConfigWrapper::setSize(unsigned w, unsigned h)
{
  Pds::PimaxConfig::setSize(_config, w, h);
}

uint32_t PimaxConfigWrapper::frameSize() const
{
  return _config.frameSize();
}

uint32_t PimaxConfigWrapper::numPixelsX() const
{
  return _config.numPixelsX();
}

uint32_t PimaxConfigWrapper::numPixelsY() const
{
  return _config.numPixelsY();
}

uint32_t PimaxConfigWrapper::numPixels() const
{
  return _config.numPixels();
}

uint16_t PimaxConfigWrapper::intensifierGain() const
{
  return _config.intensifierGain();
}

double PimaxConfigWrapper::gateDelay() const
{
  return _config.gateDelay();
}

double PimaxConfigWrapper::gateWidth() const
{
  return _config.gateWidth();
}

} //namespace Pds
