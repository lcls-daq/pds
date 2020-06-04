#include "pds/pgp/Reg.hh"
#include "pds/pgp/SrpV3.hh"
#include "pds/pgp/Destination.hh"
#include <boost/thread/tss.hpp>
#include <vector>
#include <sstream>
#include <stdio.h>

typedef Pds::Pgp::SrpV3::Protocol PgpProto;

using namespace Pds::Pgp;

static bool _debug = false;

//
//  Must use setPgp(), setDest() to switch context - not thread safe
//

boost::thread_specific_ptr<PgpProto   > _pgp;
boost::thread_specific_ptr<Destination> _dest;
#define DESTV _dest.get()
#define PGPV  _pgp.get()
#define SETDEST(d) { _dest.release(); _dest.reset(d); }
#define SETPGP(p)  { _pgp .release(); _pgp .reset(p); }

static unsigned     _tid  = 0;

void Reg::setPgp (PgpProto* p) { 
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
  unsigned errorCount = 0;
  while (true) {
    if (_pgp->readRegister(DESTV, addr, _tid++, &v)) {
      std::stringstream o;
      o << "Pgp::Reg read error @ address " << std::hex << addr
        << "[" << v << "] dest[" << std::hex << _dest->dest() << "] pgp["
        << PGPV << "] errorCount[" << ++errorCount << "]";
      printf("Reg::unsigned: %s\n",o.str().c_str());
      if (errorCount > 3)
        throw o.str();
    } else {
      return v;
    }
  }
}
