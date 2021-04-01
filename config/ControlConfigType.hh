#ifndef Pds_ControlConfigType_hh
#define Pds_ControlConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/control.ddl.h"

#include <list>

typedef Pds::ControlData::ConfigV4 ControlConfigType;
typedef Pds::ControlData::PVControlV1 PVControlType;
typedef Pds::ControlData::PVMonitorV1 PVMonitorType;
typedef Pds::ControlData::PVLabelV1 PVLabelType;

static Pds::TypeId _controlConfigType(Pds::TypeId::Id_ControlConfig,
				      ControlConfigType::Version);

static const Pds::ClockTime EndCalibTime(0,0);
static const Pds::ClockTime EndRunTime  (0,1);

static const uint32_t ControlConfigApiVersion = 1;

namespace Pds {
  namespace ControlConfig {

    class L3TEvents {
    public:
      L3TEvents(unsigned e) : _events(e) {}
      operator unsigned() const { return _events; }
    private:
      unsigned _events;
    };

    ControlConfigType* _new(void*);
    ControlConfigType* _new(void*, 
                            const std::list<PVControlType>&,
                            const std::list<PVMonitorType>&,
                            const std::list<PVLabelType>&,
                            const Pds::ClockTime&);
    ControlConfigType* _new(void*, 
                            const std::list<PVControlType>&,
                            const std::list<PVMonitorType>&,
                            const std::list<PVLabelType>&,
                            unsigned events);
    ControlConfigType* _new(void*, 
                            const std::list<PVControlType>&,
                            const std::list<PVMonitorType>&,
                            const std::list<PVLabelType>&,
                            L3TEvents events);

    size_t convert(void*,
                   const ControlConfigType&,
                   uint32_t version);

    uint32_t pvControlNameSize(uint32_t version);
    uint32_t pvMonitorNameSize(uint32_t version);
    uint32_t pvLabelNameSize  (uint32_t version);
    uint32_t pvLabelValueSize (uint32_t version);
  }
}

#endif
