/*
 * RegisterSlaveImportFrame.hh
 *
 *  Created on: Mar 30, 2010
 *      Author: jackp
 */

#ifndef PGPREGISTERSLAVEIMPORTFRAME_HH_
#define PGPREGISTERSLAVEIMPORTFRAME_HH_

#include <stdint.h>
#include "pds/pgp/RegisterSlaveExportFrame.hh"

namespace Pds {
  namespace Pgp {

    class LastBits {
      public:
        unsigned mbz2:        16;      //15:0
        unsigned failed:       1;      //16
        unsigned timeout:      1;      //17
        unsigned mbz1:        14;      //31:18
    };

    class RegisterSlaveImportFrame {
      public:
        RegisterSlaveImportFrame() {};
        ~RegisterSlaveImportFrame() {};

      public:
        unsigned tid()                                {return bits.tid;}
        unsigned addr()                               {return bits.addr;}
        RegisterSlaveExportFrame::waitState waiting() {return (RegisterSlaveExportFrame::waitState)bits.waiting;}
        unsigned lane()                               {return bits.lane;}
        unsigned vc()                                 {return bits.vc;}
        RegisterSlaveExportFrame::opcode opcode()     {return (RegisterSlaveExportFrame::opcode)bits.oc;}
        uint32_t data()                               {return _data;}
        uint32_t* array()                             {return (uint32_t*)&_data;}
        unsigned timeout()                            {return lbits.timeout;}
        unsigned failed()                             {return lbits.failed;}
        unsigned timeout(LastBits* l);
        unsigned failed(LastBits* l);
        RegisterSlaveExportFrame::FEdest   dest();
        void print(unsigned size=4);
        enum VcTypes {dataVC=3};

      public:
        SEbits    bits;
        uint32_t  _data;
        LastBits  lbits;
    };

  }
}

#endif /* PGPREGISTERSLAVEIMPORTFRAME_HH_ */
