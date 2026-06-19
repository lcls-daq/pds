#ifndef Pds_NsCam_LLNLv1_hh
#define Pds_NsCam_LLNLv1_hh

#include "pds/nscam/Board.hh"

namespace Pds {
  namespace NsCam {
    class LLNLv1 : public Board {
    public:
      LLNLv1(SensorType stype, std::shared_ptr<Comm> comm);
      virtual ~LLNLv1() = default;

      virtual void softReboot() override;
      virtual void initBoard() override;
      virtual void initPots() override;
      virtual void initSensor() override;
      virtual void latchPots() override;

      virtual void configADCs() override;

      virtual void enableLED(bool status) override;
      virtual void setLED(uint32_t led, bool status) override;

      virtual double getTemp(TempType scale) const override;
    };
  }
}

#endif
