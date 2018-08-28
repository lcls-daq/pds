#ifndef PIMAX_CONFIG_WRAPPER_HH
#define PIMAX_CONFIG_WRAPPER_HH

#include "pdsdata/xtc/Xtc.hh"
#include "pds/picam/PicamConfig.hh"
#include "pds/config/PimaxConfigType.hh"

namespace Pds
{

class CfgClientNfs;

class PimaxConfigWrapper : public PicamConfig
{
public:
  PimaxConfigWrapper(CfgClientNfs& cfg);
  ~PimaxConfigWrapper();

  PimaxConfigType& config();

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
  uint16_t intensifierGain() const;
  double gateDelay() const;
  double gateWidth() const;

private:
  CfgClientNfs&       _cfg;
  Xtc                 _cfgtc;
  PimaxConfigType     _config;

};

} //namespace Pds

#endif
