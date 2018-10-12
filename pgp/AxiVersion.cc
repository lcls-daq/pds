#include "pds/pgp/AxiVersion.hh"

std::string Pds::Pgp::AxiVersion::buildStamp() const
{
  uint32_t tmp[64];
  for(unsigned i=0; i<64; i++)
    tmp[i] = _buildStamp[i];
  return std::string(reinterpret_cast<const char*>(tmp));
}
