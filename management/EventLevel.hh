#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "pds/collection/CollectionManager.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/OutletWireInsList.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;
class EbIStream;

class EventLevel: public CollectionManager {
public:
  EventLevel(unsigned partition,
	     unsigned index,
	     EventCallback& callback,
	     Arp* arp);
  virtual ~EventLevel();
  
  void attach();
  
private:
  // Implements CollectionManager
  virtual void message  (const Node& hdr, const Message& msg);
  virtual void connected(const Node& hdr, const Message& msg);
  virtual void timedout();
  virtual void disconnected();

private:
  Node           _dissolver;        // source of resign control message
  int            _index;            // partition-wide vectoring index for receiving event data
  EventCallback& _callback;         // object to notify
  EventStreams*  _streams;          // appliance streams
  GenericPool    _pool;
  EbIStream*     _inlet;
  OutletWireInsList _rivals;        // list of nodes at this level
};

}

#endif
