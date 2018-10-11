#ifndef ArchonConfigType_hh
#define ArchonConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/archon.ddl.h"

typedef Pds::Archon::ConfigV2 ArchonConfigType;

static const unsigned ArchonConfigMaxLineLength = Pds::Archon::ConfigV2::MaxConfigLineLength;
static const unsigned ArchonConfigMaxNumLines = Pds::Archon::ConfigV2::MaxConfigLines;
static const unsigned ArchonConfigMaxFileLength = ArchonConfigMaxLineLength * ArchonConfigMaxNumLines;

static Pds::TypeId _archonConfigType(Pds::TypeId::Id_ArchonConfig,
				     ArchonConfigType::Version);

#endif
