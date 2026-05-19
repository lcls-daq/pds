#ifndef Pds_NsCam_Detector_hh
#define Pds_NsCam_Detector_hh

#include "pds/nscam/Comm.hh"
#include "pds/nscam/Board.hh"
#include "pds/nscam/Sensor.hh"

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

      void interfaceInfo() const;
      void boardInfo() const;
      void sensorInfo() const;
      std::string boardName() const;
      std::string sensorName() const;
      uint32_t boardFpgaNum() const;
      uint32_t boardFpgaRev() const;
      bool boardFpgaRad() const;
    private:
      std::shared_ptr<Comm> comm_;
      std::shared_ptr<Board> board_;
      std::shared_ptr<Sensor> sensor_;
    };
  }
}

#endif
