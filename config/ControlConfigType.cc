#include "pds/config/ControlConfigType.hh"

static bool compare_control(const PVControlType& a, const PVControlType& b) { return strcmp(a.name(),b.name())<=0; }
static bool compare_monitor(const PVMonitorType& a, const PVMonitorType& b) { return strcmp(a.name(),b.name())<=0; }
static bool compare_label  (const PVLabelType  & a, const PVLabelType  & b) { return strcmp(a.name(),b.name())<=0; }

static PVControlType* list_to_array(const std::list<PVControlType>&);
static PVMonitorType* list_to_array(const std::list<PVMonitorType>&);
static PVLabelType  * list_to_array(const std::list<PVLabelType  >&);

ControlConfigType* Pds::ControlConfig::_new(void* p)
{
  return new(p) ControlConfigType(0,0,0,1,ClockTime(0,0),0,0,0,0,0,0);
}

ControlConfigType* Pds::ControlConfig::_new(void* p, 
                                            const std::list<PVControlType>& controls,
                                            const std::list<PVMonitorType>& monitors,
                                            const std::list<PVLabelType>&   labels,
                                            const Pds::ClockTime& ctime)
{
  PVControlType* c = list_to_array(controls);
  PVMonitorType* m = list_to_array(monitors);
  PVLabelType  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(0, 0, 1, 0, ctime,
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
  return r;
}

ControlConfigType* Pds::ControlConfig::_new(void* p, 
                                            const std::list<PVControlType>& controls,
                                            const std::list<PVMonitorType>& monitors,
                                            const std::list<PVLabelType>&   labels,
                                            unsigned events)
{
  PVControlType* c = list_to_array(controls);
  PVMonitorType* m = list_to_array(monitors);
  PVLabelType  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(events, 0, 0, 1, ClockTime(0,0),
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
  return r;
}

ControlConfigType* Pds::ControlConfig::_new(void* p, 
                                            const std::list<PVControlType>& controls,
                                            const std::list<PVMonitorType>& monitors,
                                            const std::list<PVLabelType>&   labels,
                                            L3TEvents events)
{
  PVControlType* c = list_to_array(controls);
  PVMonitorType* m = list_to_array(monitors);
  PVLabelType  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(events, 1, 0, 0, ClockTime(0,0),
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
  return r;
}

size_t Pds::ControlConfig::convert(void* p,
                                   const ControlConfigType& orig,
                                   uint32_t version)
{
  using Pds::ControlData::PVControl;
  using Pds::ControlData::PVMonitor;
  using Pds::ControlData::PVLabel;

  switch (version) {
  case 0:
    { Pds::ControlData::ConfigV3* cfg = 0;
      PVControl* c = 0;
      PVMonitor* m = 0;
      PVLabel  * l = 0;

      if (orig.npvControls()!=0) {
        c = new PVControl[orig.npvControls()];
        ndarray<const ControlData::PVControlV1, 1> pvcs = orig.pvControls();
        for (unsigned i=0; i<orig.npvControls(); i++) {
          c[i] = PVControl(pvcs[i].name(),
                           pvcs[i].index(),
                           pvcs[i].value());
        }
      }

      if (orig.npvMonitors()!=0) {
        m = new PVMonitor[orig.npvMonitors()];
        ndarray<const ControlData::PVMonitorV1, 1> pvms = orig.pvMonitors();
        for (unsigned i=0; i<orig.npvMonitors(); i++) {
          m[i] = PVMonitor(pvms[i].name(),
                           pvms[i].index(),
                           pvms[i].loValue(),
                           pvms[i].hiValue());
        }
      }

      if (orig.npvLabels()!=0) {
        l = new PVLabel[orig.npvLabels()];
        ndarray<const ControlData::PVLabelV1, 1> pvls = orig.pvLabels();
        for (unsigned i=0; i<orig.npvLabels(); i++) {
          l[i] = PVLabel(pvls[i].name(),
                         pvls[i].value());
        }
      }

      cfg = new(p) Pds::ControlData::ConfigV3(orig.events(),
                                              orig.uses_l3t_events(),
                                              orig.uses_duration(),
                                              orig.uses_events(),
                                              orig.duration(),
                                              orig.npvControls(),
                                              orig.npvMonitors(),
                                              orig.npvLabels(),
                                              c, m, l);
      return cfg->_sizeof();
    }
    break;
  default:
    return 0;
  }
}

uint32_t Pds::ControlConfig::pvControlNameSize(uint32_t version)
{
  switch (version) {
  case ControlConfigApiVersion:
    return PVControlType::NameSize;
  case 0:
    return Pds::ControlData::PVControl::NameSize;
  default:
    return 0;
  }
}

uint32_t Pds::ControlConfig::pvMonitorNameSize(uint32_t version)
{
  switch (version) {
  case ControlConfigApiVersion:
    return PVMonitorType::NameSize;
  case 0:
    return Pds::ControlData::PVMonitor::NameSize;
  default:
    return 0;
  }
}

uint32_t Pds::ControlConfig::pvLabelNameSize(uint32_t version)
{
  switch (version) {
  case ControlConfigApiVersion:
    return PVLabelType::NameSize;
  case 0:
    return Pds::ControlData::PVLabel::NameSize;
  default:
    return 0;
  }
}

uint32_t Pds::ControlConfig::pvLabelValueSize(uint32_t version)
{
  switch (version) {
  case ControlConfigApiVersion:
    return PVLabelType::ValueSize;
  case 0:
    return Pds::ControlData::PVLabel::ValueSize;
  default:
    return 0;
  }
}

PVControlType* list_to_array(const std::list<PVControlType>& pvcs)
{
  PVControlType* c = 0;
  if (pvcs.size()!=0) {
    std::list<PVControlType> sorted_pvcs (pvcs); sorted_pvcs.sort(compare_control);
    c = new PVControlType[pvcs.size()];
    unsigned i=0;
    for(std::list<PVControlType>::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}

PVMonitorType* list_to_array(const std::list<PVMonitorType>& pvcs)
{
  PVMonitorType* c = 0;
  if (pvcs.size()!=0) {
    std::list<PVMonitorType> sorted_pvcs (pvcs); sorted_pvcs.sort(compare_monitor);
    c = new PVMonitorType[pvcs.size()];
    unsigned i=0;
    for(std::list<PVMonitorType>::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}

PVLabelType  * list_to_array(const std::list<PVLabelType  >& pvcs)
{
  PVLabelType  * c = 0;
  if (pvcs.size()!=0) {
    std::list<PVLabelType  > sorted_pvcs (pvcs); sorted_pvcs.sort(compare_label);
    c = new PVLabelType  [pvcs.size()];
    unsigned i=0;
    for(std::list<PVLabelType  >::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}

