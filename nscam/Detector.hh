#ifndef Pds_NsCam_Detector_hh
#define Pds_NsCam_Detector_hh

#include "pds/nscam/Comm.hh"
#include "pds/nscam/Board.hh"
#include "pds/nscam/Sensor.hh"

#include <chrono>

namespace Pds {
  namespace NsCam {
    class Detector {
    public:
      static void listDevices();
      Detector(const std::string& host,
               unsigned short port,
               CommType ctype,
               BoardType btype,
               SensorType stype);
      virtual ~Detector();

      void initialize();
      void reinitialize();
      void reboot();
      void resetTimer();

      void interfaceInfo() const;
      void boardInfo() const;
      void sensorInfo() const;
      void statusInfo() const;
      std::string boardName() const;
      std::string sensorName() const;
      uint32_t boardFpgaNum() const;
      uint32_t boardFpgaRev() const;
      bool boardFpgaRad() const;
      bool powerCheck(uint32_t delta=10) const;
      uint32_t getTimer() const;
      double getTemp(TempType scale = TempType::C) const;
      double getPressure(double offset, double sensitivity, PressureType scale) const;
      double getPressure(PressureType scale) const;
      double getPressure() const;
      uint32_t getRegister(const std::string& regname) const;
      void setRegister(const std::string& regname, uint32_t value);
      uint32_t getSubRegister(const std::string& subregname) const;
      void setSubRegister(const std::string& subregname, uint32_t value);
      double getPot(const std::string& potname) const;
      double getPotV(const std::string& potname) const;
      void setPot(const std::string& potname, double fraction=1.0);
      void setPotV(const std::string& potname, double voltage,
                   bool tune=false, double accuracy=0.01,
                   int iterations=20, double approach=0.75,
                   bool tuneErr=false);
      double getMonV(const std::string& potname) const;
      bool manualTiming() const;
      Sequence getArbTiming(SideType side) const;
      Timing getTiming(SideType side) const;
      Sequence getManualTiming(SideType side) const;
      void setArbTiming(SideType side, const Sequence& sequence);
      void setTiming(SideType side, const Timing& timing);
      void setManualTiming(SideType side, const Sequence& sequence);
    private:
      void initPowerCheck();

      std::chrono::system_clock::time_point inittime_;
      std::shared_ptr<Comm> comm_;
      std::shared_ptr<Board> board_;
      std::shared_ptr<Sensor> sensor_;
    };
  }
}

#endif
