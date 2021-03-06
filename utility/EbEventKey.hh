#ifndef Pds_EbEventKey_hh
#define Pds_EbEventKey_hh

namespace Pds {

  class Sequence;
  class AnySequenceSrv;
  class BldSequenceSrv;
  class EbSequenceSrv;
  class EbCountSrv;
  class EvrServer;

#define EbServerDeclare \
    virtual bool succeeds (EbEventKey& key) const { return key.precedes (*this); } \
    virtual bool coincides(EbEventKey& key) const { return key.coincides(*this); } \
    virtual void assign   (EbEventKey& key) const { key.assign   (*this); } \


#define EbEventKeyDeclare(server) \
    virtual bool precedes (const server &) { return false; } \
    virtual bool coincides(const server &) { return false; } \
    virtual void assign   (const server &) {} \

  class EbEventKey {
  public:
    virtual ~EbEventKey() {}
  public:
    EbEventKeyDeclare(EbSequenceSrv);
    EbEventKeyDeclare(BldSequenceSrv);
    EbEventKeyDeclare(AnySequenceSrv);
    EbEventKeyDeclare(EbCountSrv);
    EbEventKeyDeclare(EvrServer);
  public:
    virtual const Sequence& sequence() const = 0;
    virtual unsigned        value() const = 0;
  };

}

#endif
