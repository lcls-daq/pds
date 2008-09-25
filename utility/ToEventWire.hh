#ifndef PDS_TOEVENTWIRE_HH
#define PDS_TOEVENTWIRE_HH

#include <sys/uio.h>

#include "OutletWire.hh"
#include "ToNetEb.hh"

#include "OutletWireInsList.hh"
#include "pds/xtc/xtc.hh"

namespace Pds {

  class CollectionManager;
  class Outlet;
  class Datagram;

  class ToEventWire : public OutletWire {
  public:
    ToEventWire(Outlet& outlet,
		CollectionManager&,
		int);
    ~ToEventWire();

    virtual Transition* forward(Transition* dg);
    virtual InDatagram* forward(InDatagram* dg);
    virtual void bind(unsigned id, const Ins& node);
    virtual void unbind(unsigned id);
    
    // Debugging
    virtual void dump(int detail);
    virtual void dumpHistograms(unsigned tag, const char* path);
    virtual void resetHistograms();

    bool isempty() const {return _nodes.isempty();}
    
  private:
    OutletWireInsList _nodes;
    CollectionManager& _collection;
    ToNetEb _postman;
  };
}

#endif
