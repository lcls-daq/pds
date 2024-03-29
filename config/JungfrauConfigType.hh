#ifndef JungfrauConfigType_hh
#define JungfrauConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"

typedef Pds::Jungfrau::ConfigV4 JungfrauConfigType;
typedef Pds::Jungfrau::ModuleInfoV1 JungfrauModInfoType;
typedef Pds::Jungfrau::ModuleConfigV1 JungfrauModConfigType;

static Pds::TypeId _jungfrauConfigType(Pds::TypeId::Id_JungfrauConfig,
				     JungfrauConfigType::Version);

namespace Pds {
  namespace JungfrauModConfig {
    void setSerialNumber(JungfrauModConfigType&,
                         uint64_t serialNumber);
  }

  namespace JungfrauConfig {
    void setSize(JungfrauConfigType&,
                 unsigned modules,
                 unsigned rows,
                 unsigned columns,
                 JungfrauModConfigType* modcfg=NULL);
  }
}

#endif
