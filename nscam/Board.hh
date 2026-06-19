#ifndef Pds_NsCam_Board_hh
#define Pds_NsCam_Board_hh

#include "pds/nscam/Device.hh"

#include <vector>

namespace Pds {
  namespace NsCam {
    class Board : public Device {
    public:
      static std::shared_ptr<Board> create(BoardType btype, SensorType stype, std::shared_ptr<Comm> comm);
      Board(BoardType btype,
            SensorType stype,
            std::shared_ptr<Comm> comm,
            const std::map<std::string, uint16_t>& regnames,
            const std::map<std::string, SubRegister>& subregnames,
            const std::map<std::string, uint32_t>& defaults,
            const std::map<std::string, std::string>& aliases,
            const std::map<std::string, std::string>& monitors);
      virtual ~Board() = default;

      virtual void info() const;
      virtual void status() const;
      virtual uint32_t fpgaNum() const;
      virtual uint32_t fpgaRev() const;
      virtual bool fpgaRad() const;
      virtual const std::vector<CommType>& fpgaInterfaces() const;
      virtual BoardType type() const;
      virtual std::string name() const;
      virtual double vref() const;
      virtual uint32_t adc5_mult() const;
      virtual bool adc5_bipolar() const;

      virtual uint32_t getTimer() const;
      virtual void resetTimer();

      virtual double getTemp(TempType scale) const = 0;
      virtual double getPressure(double offset, double sensitivity, PressureType scale) const;
      virtual double getPressure(PressureType scale) const;
      virtual double getPressure() const;

      virtual void softReboot() = 0;
      virtual void initBoard() = 0;
      virtual void initPots() = 0;
      virtual void initSensor() = 0;
      virtual void latchPots() = 0;

      virtual void clearStatus();
      virtual uint32_t checkStatus() const;
      virtual uint32_t checkStatus2() const;
      virtual void configADCs() = 0;

      virtual void enableLED(bool status) = 0;
      virtual void setLED(uint32_t led, bool status) = 0;

    protected:
      virtual double convertMonV(double fraction) const override;

      uint32_t fpgaNum_;
      uint32_t fpgaRev_;
      uint32_t fpgaRad_;
      std::vector<CommType> fpgaInterfaces_;
      BoardType btype_;
      SensorType stype_;
      double vref_;
      uint32_t adc5_mult_;
      bool adc5_bipolar_;
    };
  }
}

#endif
