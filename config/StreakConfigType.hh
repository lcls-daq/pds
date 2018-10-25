#ifndef StreakConfigType_hh
#define StreakConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/streak.ddl.h"

typedef Pds::Streak::ConfigV1 StreakConfigType;

static Pds::TypeId _streakConfigType(Pds::TypeId::Id_StreakConfig,
				     StreakConfigType::Version);

#endif
