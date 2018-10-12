#include "pds/pgp/SrpV3.hh"
#include <stdio.h>

using namespace Pds::Pgp::SrpV3;

RegisterSlaveFrame::RegisterSlaveFrame(Pds::Pgp::PgpRSBits::opcode oc,
                                       Destination* dest,
                                       uint64_t     addr,
                                       unsigned     tid,
                                       uint32_t     size) :
  _word0   (0x03 | ((oc&3)<<8) | (0xa<<24)),
  _tid     (tid),
  _addr_lo ((addr>> 0)&0xffffffff),
  _addr_hi ((addr>>32)&0xffffffff),
  _req_size(size*4-1)  // bytes-1
{}

void RegisterSlaveFrame::print(unsigned nw) const {
  printf("Read:");
  for(unsigned i=0; i<nw; i++)
    printf(" %08x",reinterpret_cast<const uint32_t*>(this)[i]);
  printf("\n");
}
