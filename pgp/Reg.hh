#ifndef Pds_Pgp_Reg_hh
#define Pds_Pgp_Reg_hh

#include <stdint.h>

namespace Pds {
  namespace Pgp {
    class Pgp;
    class Reg {
    public:
      Reg& operator=(unsigned);
      operator unsigned() const;
    public:
      static void     setPgp (Pgp*);
      static void     setDest(unsigned);
    private:
      uint32_t _v;
    };
  };
};

#endif
