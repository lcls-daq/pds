#ifndef Pds_EpicsCamDataType_hh
#define Pds_EpicsCamDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::FrameV1 EpicsCamDataType;

static Pds::TypeId _epicsCamDataType(Pds::TypeId::Id_Frame,
                                     EpicsCamDataType::Version);

#endif
