#ifndef EpicsCamConfigType_hh
#define EpicsCamConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::ControlsCameraConfigV1 EpicsCamConfigType;

static Pds::TypeId _epicsCamConfigType(Pds::TypeId::Id_ControlsCameraConfig,
                                       EpicsCamConfigType::Version);

#endif
