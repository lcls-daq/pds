#ifndef Pds_UxiDataType_hh
#define Pds_UxiDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/uxi.ddl.h"

typedef Pds::Uxi::FrameV1 UxiDataType;

static Pds::TypeId _uxiDataType(Pds::TypeId::Id_UxiFrame,
          UxiDataType::Version);

namespace Pds {
  namespace UxiData {
    void setFrameInfo(UxiDataType&,
                      unsigned a,
                      unsigned t);

    void setTemperature(UxiDataType&,
                        double t);
  }
}

#endif

