#ifndef PIXIS_CONFIG_WRAPPER_HH
#define PIXIS_CONFIG_WRAPPER_HH

#include "pdsdata/xtc/Xtc.hh"
#include "pds/picam/PicamConfig.hh"
#include "pds/config/PixisConfigType.hh"

namespace Pds
{

class CfgClientNfs;

class PixisConfigWrapper : public PicamConfig
{
public:
  PixisConfigWrapper(CfgClientNfs& cfg);
  ~PixisConfigWrapper();

  PixisConfigType& config();

  int fetch(Transition* tr);
  void insert(InDatagram* in);

  uint32_t width() const;
  uint32_t height() const;
  uint32_t orgX() const;
  uint32_t orgY() const;
  uint32_t binX() const;
  uint32_t binY() const;

  float exposureTime() const;
  float coolingTemp() const;
  float readoutSpeed() const;
  uint16_t gainIndex() const;

  int16_t infoReportInterval() const;
  uint16_t exposureEventCode() const;
  uint32_t numIntegrationShots() const;
  void setNumIntegrationShots(unsigned n);
  void setSize(unsigned w, unsigned h);

  uint32_t frameSize() const;
  uint32_t numPixelsX() const;
  uint32_t numPixelsY() const;
  uint32_t numPixels() const;

  /* Camera specific configs */
  uint16_t adcMode() const;
  uint16_t triggerMode() const;
  double vsSpeed() const;

private:
  CfgClientNfs&       _cfg;
  Xtc                 _cfgtc;
  PixisConfigType     _config;

};

} //namespace Pds

#endif
