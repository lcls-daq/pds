#ifndef Pds_VimbaDataType_hh
#define Pds_VimbaDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/vimba.ddl.h"

typedef Pds::Vimba::FrameV1 VimbaDataType;

static Pds::TypeId _vimbaDataType(Pds::TypeId::Id_VimbaFrame,
                                  VimbaDataType::Version);

#endif
