#ifndef Pds_NsCam_Icarus2_hh
#define Pds_NsCam_Icarus2_hh

#include "pds/nscam/Sensor.hh"

namespace Pds {
  namespace NsCam {
    class Icarus2 : public Sensor {
    public:
      Icarus2(BoardType btype, std::shared_ptr<Comm> comm);
      virtual ~Icarus2() = default;
    };
  }
}

#endif
