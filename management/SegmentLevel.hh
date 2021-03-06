#ifndef Pds_SegmentLevel_hh
#define Pds_SegmentLevel_hh

#include "PartitionMember.hh"
#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"

namespace Pds {

  class SegWireSettings;
  class WiredStreams;
  class Arp;
  class EventCallback;
  class Server;

  class SegmentLevel: public PartitionMember {
  public:
    SegmentLevel(unsigned platform,
     SegWireSettings& settings,
     EventCallback& callback,
     Arp* arp,
     int slowEb = 0
    );

    virtual ~SegmentLevel();

    bool attach();
    void detach();
  private:
    // Implements PartitionMember
    Message& reply     (Message::Type);
  protected:
    void     allocated (const Allocation&, unsigned);
    void     dissolved ();
  private:
    void     post      (const Transition&);
    void     post      (const Occurrence&);
    void     post      (const InDatagram&);

  protected:
    SegWireSettings& _settings;
    EventCallback& _callback;
    WiredStreams*  _streams;
    Server*        _evr;
    PingReply      _reply;
    AliasReply     _aliasReply;
  };

}
#endif
