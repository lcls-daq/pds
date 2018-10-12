#ifndef Pds_Pgp_AxiVersion_hh
#define Pds_Pgp_AxiVersion_hh

#include "pds/pgp/Reg.hh"

#include <string>

namespace Pds {
  namespace Pgp {
    class AxiVersion {
    public:
      Reg _fwVersion;
      Reg _scratch;
      Reg _upTime;
      uint32_t reserved_0x100[(0x100-0xc)>>2];
      Reg _fpgaReloadHalt;
      Reg _fpgaReload;
      Reg _fpgaReloadAddr;
      Reg _userReset;
      uint32_t reserved_0x300[(0x300-0x110)>>2];
      Reg _fdSerial[2];
      uint32_t reserved_0x400[(0x400-0x308)>>2];
      Reg _userConstants;
      uint32_t reserved_0x500[(0x500-0x404)>>2];
      Reg _deviceId;
      uint32_t reserved_0x600[(0x600-0x504)>>2];
      Reg _gitHash[160>>2];
      uint32_t reserved_0x700[(0x700-0x6a0)>>2];
      Reg _deviceDna[4];
      uint32_t reserved_0x800[(0x800-0x710)>>2];
      Reg _buildStamp[64];
    public:
      std::string buildStamp() const;
    };
  };
};

#endif
