#ifndef Pds_UxiConfigType_hh
#define Pds_UxiConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/uxi.ddl.h"

typedef Pds::Uxi::ConfigV2 UxiConfigType;

static Pds::TypeId _uxiConfigType(Pds::TypeId::Id_UxiConfig,
          UxiConfigType::Version);

static const unsigned UxiConfigNumberOfPots = Pds::Uxi::ConfigV2::NumberOfPots;
static const unsigned UxiConfigNumberOfSides = Pds::Uxi::ConfigV2::NumberOfSides;

namespace Pds {
  namespace UxiConfig {
    void setPots(UxiConfigType&,
                 double* p);
    void setRoi(UxiConfigType&,
                unsigned fr,
                unsigned lr,
                unsigned ff,
                unsigned lf);
    void setSize(UxiConfigType&,
                 unsigned w,
                 unsigned h,
                 unsigned n,
                 unsigned b,
                 unsigned t);
  }
}

#endif
