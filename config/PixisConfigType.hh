#ifndef Pds_PixisConfigType_hh
#define Pds_PixisConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/pixis.ddl.h"

typedef Pds::Pixis::ConfigV1 PixisConfigType;

static Pds::TypeId _pixisConfigType(Pds::TypeId::Id_PixisConfig,
          PixisConfigType::Version);

namespace Pds {
  namespace PixisConfig {
    void setNumIntegrationShots(PixisConfigType&,
                          unsigned);
    void setSize(PixisConfigType&,
                 unsigned w,
                 unsigned h);
  }
}

#endif
