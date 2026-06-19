#ifndef Pds_NsCam_LLNLv4_hh
#define Pds_NsCam_LLNLv4_hh

#include "pds/nscam/Board.hh"

namespace Pds {
  namespace NsCam {
    class LLNLv4 : public Board {
    public:
      LLNLv4(SensorType stype, std::shared_ptr<Comm> comm);
      virtual ~LLNLv4() = default;

      virtual void softReboot() override;
      virtual void initBoard() override;
      virtual void initPots() override;
      virtual void initSensor() override;
      virtual void latchPots() override;

      virtual void configADCs() override;

      virtual void enableLED(bool status) override;
      virtual void setLED(uint32_t led, bool status) override;

      virtual double getTemp(TempType scale) const override;
      virtual double getPressure(double offset, double sensitivity, PressureType scale) const override;
      virtual double getPressure(PressureType scale) const override;
      virtual double getPressure() const override;

    private:
      double def_pressure_off_;
      double def_pressure_sen_;
    };
  }
}

#endif
