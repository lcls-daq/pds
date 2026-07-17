#ifndef Pds_UxiConfigType_hh
#define Pds_UxiConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/uxi.ddl.h"

typedef Pds::Uxi::ConfigV4 UxiConfigType;

static Pds::TypeId _uxiConfigType(Pds::TypeId::Id_UxiConfig,
          UxiConfigType::Version);

static const unsigned UxiConfigNumberOfPots = Pds::Uxi::ConfigV4::NumberOfPots;
static const unsigned UxiConfigNumberOfSides = Pds::Uxi::ConfigV4::NumberOfSides;
static const unsigned UxiConfigMaxManualShutterSequence = Pds::Uxi::ConfigV4::MaxManualShutterSequence;
static const unsigned UxiConfigMaxTimingSequence = Pds::Uxi::ConfigV4::MaxTimingSequence;
/* ddl supports infinite Registers but the rest of the code needs a max because of how buffers are allocated for configs
 * this can be changed to any higher number that is needed! */
static const unsigned UxiConfigMaxRegisters = 200;

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
