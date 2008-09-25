#include "EbC.hh"

#include "pds/xtc/CDatagram.hh"
#include "EbCountKey.hh"
#include "EbEvent.hh"

using namespace Pds;

EbC::EbC(const Src& id,
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
  _keys( sizeof(EbCountKey), eventpooldepth)
{
}

EbC::~EbC()
{
}

EbEventBase* EbC::_new_event(const EbBitMask& serverId)
{
  unsigned depth = _datagrams.numberofObjects()+_datagrams.numberofFrees()-_datagrams.numberofAllocs();

#ifdef USE_VMON
  if (_vmoneb.status() == VmonManager::Running) {
    _vmoneb.dginuse(depth);
  }
#endif

  if (!depth) {
    _postEvent(_pending.forward());
  }

  CDatagram* datagram = new(&_datagrams) CDatagram(_header, _id);
  EbCountKey* key = new(&_keys) EbCountKey(const_cast<Datagram&>(datagram->datagram()));
  return new(&_events) EbEvent(serverId, _clients, datagram, key);
}


