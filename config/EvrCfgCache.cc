#include "EvrCfgCache.hh"

#include "pds/utility/Transition.hh"
#include "pds/collection/Node.hh"
#include "pds/config/EvrConfigType.hh"

static const Pds::DetInfo info = Pds::DetInfo(0, Pds::DetInfo::NoDetector, 0, Pds::DetInfo::Evr,0);
static const unsigned evrConfigSize = sizeof(EvrConfigType) + 64 * sizeof(EventCodeType) + 10 * sizeof(PulseType) + 80 * sizeof(OutputMapType);

using namespace Pds;

EvrCfgCache::EvrCfgCache() :
  CfgCache(info, _evrConfigType, evrConfigSize),
  _group(0)
{}

EvrCfgCache::~EvrCfgCache()
{}

void EvrCfgCache::init(const Allocation& alloc)
{
  CfgCache::init(alloc);
  int pid = getpid();
  for (unsigned n = 0; n < alloc.nnodes(); n++) {
    const Node *node = alloc.node(n);
    if (node->level() == Level::Segment && node->pid() == pid) {
      _group = node->group();
      break;
    }
  }
}

int EvrCfgCache::_size (void* buffer) const
{
  const EvrConfigType* c = reinterpret_cast<const EvrConfigType*>(buffer);
  if (c)
    return c->_sizeof();
  else
    return 0;
}

unsigned EvrCfgCache::code() const
{ 
  const EvrConfigType* c = reinterpret_cast<const EvrConfigType*>(current());
  if (c) {
    ndarray<const EventCodeType, 1> codes = c->eventcodes();
    for(unsigned k=0; k<c->neventcodes(); k++) {
      const EventCodeType& ec = c->eventcodes()[k];
      if (ec.isReadout() && ec.readoutGroup() == _group)
        return ec.code();
    }
  }

  return 0;
}

unsigned EvrCfgCache::group() const
{
  return _group;
}

