#ifndef Pds_NsCam_Daedalus_hh
#define Pds_NsCam_Daedalus_hh

#include "pds/nscam/Sensor.hh"

namespace Pds {
  namespace NsCam {
    class Daedalus : public Sensor {
    public:
      Daedalus(std::shared_ptr<Board> board);
      virtual ~Daedalus() = default;

      virtual bool checkSensorVoltStat();
    };
  }
}

#endif
