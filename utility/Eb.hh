#ifndef PDS_EB_HH
#define PDS_EB_HH

//#define USE_VMON

#include "EbBase.hh"

#include "EbEvent.hh"
#include "EbTimeouts.hh"
#ifdef USE_VMON
#include "VmonEb.hh"
#endif

#include "pds/service/GenericPoolW.hh"

namespace Pds {

class OutletWire;
class EbServer;
class Client;

class Eb : public EbBase 
  {
  public:
    Eb(const Src& id,
       const TypeId& ctns,
       Level::Type level,
       Inlet& inlet,
       OutletWire& outlet,
       int stream,
       int ipaddress,
       unsigned eventsize,
       unsigned eventpooldepth,
#ifdef USE_VMON
       const VmonEb& vmoneb,
#endif
       const Ins* dstack=0);
    virtual ~Eb();
  public:
    int  processIo(Server*);
  private:
    unsigned     _fixup      ( EbEventBase*, const Src&, const EbBitMask& );
    void         _dump       ( int detail );
  protected:
    GenericPoolW _datagrams;    // Datagram freelist
    GenericPool  _events;
  };
}
#endif
