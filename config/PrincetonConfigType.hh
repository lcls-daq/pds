#ifndef Pds_PrincetonConfigType_hh
#define Pds_PrincetonConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/princeton/ConfigV1.hh"

typedef Pds::Princeton::ConfigV1 PrincetonConfigType;

static Pds::TypeId _princetonConfigType(Pds::TypeId::Id_PrincetonConfig,
          PrincetonConfigType::Version);

#endif
