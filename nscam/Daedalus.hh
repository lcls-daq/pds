#ifndef Pds_NsCam_Daedalus_hh
#define Pds_NsCam_Daedalus_hh

#include "pds/nscam/Sensor.hh"

namespace Pds {
  namespace NsCam {
    class Daedalus : public Sensor {
    public:
      Daedalus(BoardType btype, std::shared_ptr<Comm> comm);
      virtual ~Daedalus() = default;
    };
  }
}

#endif
