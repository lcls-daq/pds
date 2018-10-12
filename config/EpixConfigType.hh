#ifndef Pds_EpixConfigType_hh
#define Pds_EpixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigV1.hh"
#include "pds/config/EpixASICConfigV1.hh"
#include "pds/config/Epix10kConfigV1.hh"
#include "pds/config/Epix10kASICConfigV1.hh"
#include "pds/config/Epix100aConfigV1.hh"
#include "pds/config/Epix100aConfigV2.hh"
#include "pds/config/Epix100aASICConfigV1.hh"
#include "pds/config/Epix10kaConfigV1.hh"
#include "pds/config/Epix10kaASICConfigV1.hh"
#include "pds/config/Epix10ka2MConfigV1.hh"

#include <string>

typedef Pds::Epix::ConfigV1 EpixConfigType;
typedef Pds::EpixConfig::ConfigV1 EpixConfigShadow;
typedef Pds::EpixConfig::ASIC_ConfigV1 EpixASIC_ConfigShadow;

static Pds::TypeId _epixConfigType(Pds::TypeId::Id_EpixConfig,
                                    EpixConfigType::Version);

typedef Pds::Epix::Config10KV1 Epix10kConfigType;
typedef Pds::Epix10kConfig::ConfigV1 Epix10kConfigShadow;
typedef Pds::Epix10kConfig::ASIC_ConfigV1 Epix10kASIC_ConfigShadow;

static Pds::TypeId _epix10kConfigType(Pds::TypeId::Id_Epix10kConfig,
                                    Epix10kConfigType::Version);

typedef Pds::Epix100aConfig_V1::ConfigV1 Epix100aConfigShadowV1;
typedef Pds::Epix::Config100aV2 Epix100aConfigType;
typedef Pds::Epix100aConfig::ConfigV2 Epix100aConfigShadow;
typedef Pds::Epix100aConfig::ASIC_ConfigV1 Epix100aASIC_ConfigShadow;

static Pds::TypeId _epix100aConfigType(Pds::TypeId::Id_Epix100aConfig,
                                    Epix100aConfigType::Version);

typedef Pds::Epix::Config10kaV1 Epix10kaConfigType;
typedef Pds::Epix10kaConfig::ConfigV1 Epix10kaConfigShadow;
typedef Pds::Epix10kaConfig::ASIC_ConfigV1 Epix10kaASIC_ConfigShadow;

static Pds::TypeId _epix10kaConfigType(Pds::TypeId::Id_Epix10kaConfig,
                                    Epix10kaConfigType::Version);

typedef Pds::Epix::Config10ka2MV1   Epix10ka2MConfigType;
typedef Pds::Epix::Config10kaQuadV1 Epix10kaQuadConfigType;
typedef Pds::Epix10ka2m::ConfigV1   Epix10ka2MConfigShadow;

static Pds::TypeId _epix10ka2MConfigType(Pds::TypeId::Id_Epix10ka2MConfig,
                                         Epix10ka2MConfigType::Version);


#endif
