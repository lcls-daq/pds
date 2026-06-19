#ifndef Pds_NsCam_Icarus_hh
#define Pds_NsCam_Icarus_hh

#include "pds/nscam/Sensor.hh"

namespace Pds {
  namespace NsCam {
    class Icarus : public Sensor {
    public:
      Icarus(std::shared_ptr<Board> board);
      virtual ~Icarus() = default;

      virtual bool checkSensorVoltStat();
    };
  }
}

#endif
