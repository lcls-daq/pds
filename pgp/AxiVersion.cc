#include "pds/pgp/AxiVersion.hh"

#include <stdio.h>

std::string Pds::Pgp::AxiVersion::gitHash() const
{
  uint32_t val;
  char tmp[64];
  for (unsigned i=0; i<5; i++) {
    val = _gitHash[4-i];
    ::sprintf(tmp + i*8, "%.08x", val);
  }
  return std::string(tmp);
}

std::string Pds::Pgp::AxiVersion::buildStamp() const
{
  uint32_t tmp[64];
  for(unsigned i=0; i<64; i++)
    tmp[i] = _buildStamp[i];
  return std::string(reinterpret_cast<const char*>(tmp));
}
