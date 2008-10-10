/*
** ++
**  Package:
**	odfConcentrator
**
**  Abstract:
**
**      This class defines the context necessary to manage an event
**      as it it makes the phase changes between "construction",
**      "completing", and "completion".
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 1,1998
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_EBEVENT_HH
#define PDS_EBEVENT_HH

#include "EbEventBase.hh"
#include "EbSegment.hh"
#include "pds/xtc/CDatagram.hh"

namespace Pds {

class EbEventKey;
class EbServer;
class EbTimeouts;
class CDatagram;

class EbEvent : public EbEventBase
  {
  public:
    EbEvent();
    EbEvent(EbBitMask creator, EbBitMask contract, 
	    CDatagram*, EbEventKey*);
   ~EbEvent();
  public:
    PoolDeclare;
  public:
    EbSegment* consume(const EbServer*,
		       int sizeofPayload,
		       EbBitMask client);
  public:
    char*         recopy    (char* payload, 
			     int sizeofPayload, 
			     EbBitMask server);
    unsigned      fixup     (const Src&, const TypeId&);
    char*         payload   (EbBitMask client);
    EbSegment*    hasSegment(EbBitMask client);

    CDatagram*    cdatagram() const;
    InDatagram*   finalize ();
  private:
    CDatagram* _cdatagram;
    LinkedList<EbSegment> _pending;  // Listhead, Segments pending fragments
    char* _segments;      // Next segment to allocate
    char  _pool[sizeof(EbSegment)*32]; // buffer for segments
  };
}

/*
** ++
**
**    The EB server allocates memory for its payload by calling this
**    function. Note, that memory is allocated one contribution ahead.
**    The memory for the payload comes from the datagram associated
**    with the event. This in turn means an "allocation" from the
**    datagram's Tagged Container. Note, that something a little chancy
**    is going on here as the memory is not actually allocated or reserved,
**    as we have no idea apriori of how much memory we actually need until
**    the read actually completes. However, the datagram assures us that
**    enough memory exists to hold the MAXIMUM contribution. Therefore,
**    we simply derive a pointer to where the payload should go. The
**    memory is actually allocated by the "consume" function (via the
**    "EbServer's" "commit" function. This is, of course, not kosher
**    if this code is not single threaded and another payload request
**    sneaks in, BEFORE the "consume" function is called. Fortunately,
**    "Server" guarantees this behaviour.
**
** --
*/

inline char* Pds::EbEvent::payload(EbBitMask client)
  {
  Pds::EbSegment* segment = hasSegment(client);
  if (segment) return segment->payload();
  Datagram& dg = const_cast<Datagram&>(_cdatagram->datagram());
  return (char*)dg.xtc.next();
  }

inline Pds::CDatagram* Pds::EbEvent::cdatagram() const
{
  return _cdatagram;
}

/*
** ++
**
**    As soon as an event becomes "complete" its datagram is the only
**    information of value within the event. Therefore, when the event
**    becomes complete it is deleted which cause the destructor to
**    remove the event from the pending queue.
**
** --
*/

inline Pds::EbEvent::~EbEvent()
{
}

#endif
