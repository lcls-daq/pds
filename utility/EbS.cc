#include "EbS.hh"

#include "EbEvent.hh"
#include "EbSequenceKey.hh"

using namespace Pds;

EbS::EbS(const Src& id,
	 Level::Type level,
	 Inlet& inlet,
	 OutletWire& outlet,
	 int stream,
	 int ipaddress,
	 unsigned eventsize,
	 unsigned eventpooldepth) :
  Eb(id, level, inlet, outlet,
     stream, ipaddress,
     eventsize,
     eventpooldepth),
  _keys( sizeof(EbSequenceKey), eventpooldepth )
{
  memset(_no_builds,0,sizeof(_no_builds));
}

EbS::~EbS()
{
}

void EbS::no_build(Sequence::Type type, unsigned mask)
{
  _no_builds[type] |= mask;
}

EbEventBase* EbS::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _events.numberofObjects()+_events.numberofFrees()-_events.numberofAllocs();

#ifdef USE_VMON
  if (_vmoneb.status() == VmonManager::Running) {
    _vmoneb.dginuse(depth);
  }
#endif

  if (!depth)
    _postEvent(_pending.forward());

  CDatagram* datagram = new(&_datagrams) CDatagram(_header, _id);
  EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()));
  return new(&_events) EbEvent(serverId, _clients, datagram, key);
}


bool EbS::_is_complete( EbEventBase* event,
			const EbBitMask& serverId)
{
  const Sequence& seq = event->key().sequence();
  if (_no_builds[seq.type()] & (1<<seq.service())) {
    event->remaining(event->remaining(serverId));
    return true;
  }
  return EbBase::_is_complete(event, serverId);
}
