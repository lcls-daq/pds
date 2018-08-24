#ifndef Pds_PixisDataType_hh
#define Pds_PixisDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/pixis.ddl.h"

typedef Pds::Pixis::FrameV1 PixisDataType;

static Pds::TypeId _pixisDataType(Pds::TypeId::Id_PixisFrame,
          PixisDataType::Version);

namespace Pds {
  namespace PixisData {
    void setTemperature(PixisDataType&,
                        float);
  }
}

#endif
