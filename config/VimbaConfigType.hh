#ifndef VimbaConfigType_hh
#define VimbaConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/vimba.ddl.h"

typedef Pds::Vimba::AlviumConfigV1 AlviumConfigType;

static Pds::TypeId _alviumConfigType(Pds::TypeId::Id_AlviumConfig,
                                     AlviumConfigType::Version);

namespace Pds {
  namespace VimbaConfig {
    void setROI(AlviumConfigType&,
                uint32_t x,
                uint32_t y,
                uint32_t w,
                uint32_t h);

    void setSensorInfo(AlviumConfigType&,
                       uint32_t w,
                       uint32_t h);

    void setDeviceInfo(AlviumConfigType&,
                       const char* manufacturer,
                       const char* family,
                       const char* model,
                       const char* manufacturerId,
                       const char* version,
                       const char* serialNumber,
                       const char* firmwareId,
                       const char* firmwareVersion);
  }
}

#endif
