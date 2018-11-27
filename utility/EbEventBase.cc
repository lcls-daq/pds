#include "EbEventBase.hh"
#include "EbServer.hh"
#include "EbTimeouts.hh"
#include "pds/service/SysClk.hh"

static const int MaxTimeouts=0xffff;

//#define VERBOSE

using namespace Pds;

/*
** ++
**
**   The creation of an event is initiated by the event builder, whose
**   identity is passed and saved by the event. The builder provides
**   the set of participants for this event. In addition, the builder
**   provides a buffer to contain the datagram representing the event.
**   This datagram is initialized to "empty" to represent an event
**   which has been allocated but not yet constructed. It will be
**   constructed when the first contribution sent by one of its
**   participants is received (see "consume" below). An event which
**   is pending construction is initially located on the "pending
**   construction" queue.
**   In addition, to handle the management of fragments the event contains
**   various structures including: a buffer area from which to allocate
**   segment descriptors ("_pool" and "_segments") and the listhead
**   ("_pending") of segment descriptors outstanding.
**
** --
*/

EbEventBase::EbEventBase(EbBitMask creator,
       EbBitMask contract,
       Datagram* datagram,
       EbEventKey* key) :
  _key              (key),
  _allocated        (creator),
  _contributions    (contract),
  _contract         (contract),
  _segments         (),
  _timeouts         (MaxTimeouts),
  _datagram         (datagram),
  _begin            (SysClk::sample()),
  _bClientGroupSet  (false),
  _post             (false)
  {
  }

/*
** ++
**
**   This constructor is special in the sense that it does not actually
**   allocate and initialize an event. It instead, serves to initialize
**   the listhead which identifies the list of pending events. By having
**   the listhead as an event itself, various functions which must traverse
**   this list, can be written in a much more generic fashion. Note, that
**   the allocated list is set completely FULL, to spoof the "nextEvent"
**   function from attempting to allocate the listhead.
**
** --
*/

EbEventBase::EbEventBase() :
  LinkedList<EbEventBase>(), 
  _key              (0),
  _allocated        (EbBitMask(EbBitMask::FULL)),
  _contributions    (),
  _contract         (),
  _segments         (),
  _timeouts         (MaxTimeouts),
  _datagram         (0),
  _begin            (0),
  _bClientGroupSet  (false),
  _post             (false)
  {
  }

/*
** ++
**
**    
**
** --
*/

int EbEventBase::timeouts(const EbTimeouts& ebtmo) {
  if (_timeouts == MaxTimeouts) {
    int tmo = ebtmo.timeouts(_datagram ? &_datagram->seq : 0);
    _timeouts = tmo < MaxTimeouts ? tmo : MaxTimeouts;
  }
  return --_timeouts;
}


void EbEventBase::setClientGroup(EbBitMask maskClientGroup)
{
  remaining(~maskClientGroup);
  _bClientGroupSet = true;  
}

bool EbEventBase::isClientGroupSet()
{
  return _bClientGroupSet;
}
