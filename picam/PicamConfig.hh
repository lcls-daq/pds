#ifndef PICAM_CONFIG_HH
#define PICAM_CONFIG_HH

#include <stdint.h>

namespace Pds
{

class Transition;
class InDatagram;

class PicamConfig
{
public:
  virtual ~PicamConfig() {}

  virtual int fetch(Transition* tr) = 0;
  virtual void insert(InDatagram* in) = 0;

  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual uint32_t orgX() const = 0;
  virtual uint32_t orgY() const = 0;
  virtual uint32_t binX() const = 0;
  virtual uint32_t binY() const = 0;

  virtual float exposureTime() const = 0;
  virtual float coolingTemp() const = 0;
  virtual float readoutSpeed() const = 0;
  virtual uint16_t gainIndex() const = 0;

  virtual uint16_t exposureEventCode() const = 0;
  virtual uint32_t numIntegrationShots() const = 0;
  virtual void setNumIntegrationShots(unsigned n) = 0;
  virtual void setSize(unsigned w, unsigned h) = 0;

  virtual uint32_t frameSize() const = 0;
  virtual uint32_t numPixelsX() const = 0;
  virtual uint32_t numPixelsY() const = 0;
  virtual uint32_t numPixels() const = 0;

};

} //namespace Pds

#endif
