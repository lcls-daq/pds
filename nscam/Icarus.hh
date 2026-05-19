#ifndef Pds_NsCam_Icarus_hh
#define Pds_NsCam_Icarus_hh

#include "pds/nscam/Sensor.hh"

namespace Pds {
  namespace NsCam {
    class Icarus : public Sensor {
    public:
      Icarus(BoardType btype, std::shared_ptr<Comm> comm);
      virtual ~Icarus() = default;
    };
  }
}

#endif
