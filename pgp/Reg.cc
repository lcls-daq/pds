#include "pds/pgp/Reg.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/Destination.hh"
#include <vector>
#include <sstream>
#include <stdio.h>
//#include <boost/thread/tss.hpp>

using namespace Pds::Pgp;

static bool _debug = false;

//
//  Must use setPgp(), setDest() to switch context - not thread safe
//

//boost::thread_specific_ptr<Pgp        > _pgp;
//boost::thread_specific_ptr<Destination> _dest;
static Pgp*         _pgp  = 0;
static Destination* _dest = 0;
static unsigned     _tid  = 0;

void Reg::setPgp (Pgp* p) { 
  //  _pgp.reset(p);
  _pgp = p;
}

void Reg::setDest(unsigned d) {
  // if (!_dest.get())
  //   _dest.reset(new Destination);
  if (!_dest)
    _dest = new Destination;
  *_dest = Destination(d);
}

// write register
Reg& Reg::operator=(unsigned v) {
  uint64_t addr = reinterpret_cast<uint64_t>(this);
  if (_debug)
    printf("Reg::write @%08x %08x\n", unsigned(addr),v);
  _pgp->writeRegister(_dest, addr, v);
  return *this;
}

// read register
Reg::operator unsigned() const {
  uint64_t addr = reinterpret_cast<uint64_t>(this);
  uint32_t v;
  if (_pgp->readRegister(_dest, addr, _tid++, &v)!=Pgp::Pgp::Success) {
    std::stringstream o;
    o << "Pgp::Reg read error @ address " << std::hex << addr << " [" << v << "]";
    printf("Reg::unsigned: %s\n",o.str().c_str());
    throw o.str();
  }
  return v;
}
