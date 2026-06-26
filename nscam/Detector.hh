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
               SensorType stype,
               bool init=true);
      virtual ~Detector();

      /* init functions */
      bool isInitialized() const;
      void initialize();
      void reinitialize();
      void reboot();
      void resetTimer();

      /* info and status */
      void interfaceInfo() const;
      void boardInfo() const;
      void sensorInfo() const;
      void statusInfo() const;
      void potInfo() const;
      BoardType boardType() const;
      SensorType sensorType() const;
      std::string boardName() const;
      std::string sensorName() const;
      uint32_t boardFpgaNum() const;
      uint32_t boardFpgaRev() const;
      bool boardFpgaRad() const;
      bool powerCheck(uint32_t delta=10) const;
      uint32_t getTimer() const;
      std::string host() const;
      unsigned short port() const;

      /* env sensor readback */
      double getTemp(TempType scale = TempType::C) const;
      double getPressure(double offset, double sensitivity, PressureType scale) const;
      double getPressure(PressureType scale) const;
      double getPressure() const;

      /* fixed sensor values */
      uint32_t minframe() const;
      uint32_t maxframe() const;
      uint32_t maxwidth() const;
      uint32_t maxheight() const;
      uint32_t bytesperpixel() const;

      /* current roi values */
      uint32_t nframes() const;
      uint32_t firstframe() const;
      uint32_t lastframe() const;
      uint32_t firstrow() const;
      uint32_t lastrow() const;
      size_t width() const;
      size_t height() const;
      size_t npixels() const;
      size_t payloadSize() const;
      void setRows();
      void setRows(uint32_t minrow, uint32_t maxrow);
      void setFrames();
      void setFrames(uint32_t minframe, uint32_t maxframe);

      /* Register read/write functions */
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

      /* sensor/timing settings */
      bool manualTiming() const;
      Sequence getArbTiming(SideType side) const;
      Timing getTiming(SideType side) const;
      Sequence getActualTiming(SideType side) const;
      Sequence getManualTiming(SideType side) const;
      void setArbTiming(SideType side, const Sequence& sequence);
      void setTiming(SideType side, const Timing& timing);
      void setManualTiming(SideType side, const Sequence& sequence);

      /* misc sensor settings */
      void setOscillator(OscillatorType osc=OscillatorType::RELAXATION);
      OscillatorType getOscillator() const;
      void setInterlacing(SideType side);
      void setInterlacing(uint32_t ifactor, SideType side);
      uint32_t getInterlacing(SideType side) const;
      void setHighFullWell(bool flag);
      bool getHighFullWell() const;
      void setZeroDeadTime(bool flag, SideType side);
      bool getZeroDeadTime(SideType side) const;
      void setTriggerDelay();
      void setTriggerDelay(double delay);
      double getTriggerDelay() const;
      void setPhiDelay(SideType side);
      void setPhiDelay(double delay, SideType side);
      double getPhiDelay(SideType side);
      void setExtClk();
      void setExtClk(uint32_t dilation, double frequency);

      /* readout related functions */
      bool armed() const;
      void arm(TriggerType mode=TriggerType::HARDWARE);
      void disarm();
      void startCapture(TriggerType mode=TriggerType::HARDWARE);
      bool waitForSRAM(uint32_t timeout_ms=0);
      std::unique_ptr<uint8_t[]>  readFrame8();
      std::unique_ptr<uint16_t[]> readFrame16();
      std::unique_ptr<uint32_t[]> readFrame32();
      std::unique_ptr<uint8_t[]>  waitFrame8(TriggerType mode=TriggerType::HARDWARE, uint32_t timeout_ms=0);
      std::unique_ptr<uint16_t[]> waitFrame16(TriggerType mode=TriggerType::HARDWARE, uint32_t timeout_ms=0);
      std::unique_ptr<uint32_t[]> waitFrame32(TriggerType mode=TriggerType::HARDWARE, uint32_t timeout_ms=0);
      bool abortReadoff(bool flag=true);

    private:
      void initPowerCheck();

      bool initialized_;
      std::chrono::system_clock::time_point inittime_;
      std::shared_ptr<Comm> comm_;
      std::shared_ptr<Board> board_;
      std::shared_ptr<Sensor> sensor_;
    };
  }
}

#endif
