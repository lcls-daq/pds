#include "pds/pgp/Reg.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/Destination.hh"
#include <vector>
#include <sstream>
#include <stdio.h>

//#define USE_TSS

#ifdef USE_TSS
#include <boost/thread/tss.hpp>
#endif

using namespace Pds::Pgp;

static bool _debug = false;

//
//  Must use setPgp(), setDest() to switch context - not thread safe
//

#ifdef USE_TSS
boost::thread_specific_ptr<Pgp        > _pgp;
boost::thread_specific_ptr<Destination> _dest;
#define DESTV _dest.get()
#define SETDEST(d) _dest.reset(d)
#define SETPGP(p) _pgp.reset(p)
#else
static Pgp*         _pgp  = 0;
static Destination* _dest = 0;
#define DESTV _dest
#define SETDEST(d) _dest = d 
#define SETPGP(p) _pgp = p
#endif
static unsigned     _tid  = 0;

void Reg::setPgp (Pgp* p) { 
  SETPGP(p);
}

void Reg::setDest(unsigned d) {
  if (!DESTV) SETDEST(new Destination);
  *_dest = Destination(d);
}

// write register
Reg& Reg::operator=(unsigned v) {
  uint64_t addr = reinterpret_cast<uint64_t>(this);
  if (_debug)
    printf("Reg::write @%08x %08x %x\n", unsigned(addr),v,_dest->dest());
  _pgp->writeRegister(DESTV, addr, v);
  return *this;
}

// read register
Reg::operator unsigned() const {
  uint64_t addr = reinterpret_cast<uint64_t>(this);
  uint32_t v;
  if (_pgp->readRegister(DESTV, addr, _tid++, &v)!=Pgp::Pgp::Success) {
    std::stringstream o;
    o << "Pgp::Reg read error @ address " << std::hex << addr << "[" << v << "] dest[" << std::hex << _dest->dest() << "]";
    printf("Reg::unsigned: %s\n",o.str().c_str());
    throw o.str();
  }
  return v;
}
