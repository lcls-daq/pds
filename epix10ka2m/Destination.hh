/*
 * Epix10kaDestination.hh
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#ifndef Pds_Epix10ka2m_Destination_hh
#define Pds_Epix10ka2m_Destination_hh

#include "pds/pgp/Destination.hh"

namespace Pds {

  namespace Epix10ka2m {

    class Destination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data=0, Registers=1, Oscilloscope=2, NumberOf=3};

        Destination() {}
        Destination(unsigned lane, FEdest vc) : 
          Pgp::Destination((lane<<2)|unsigned(vc)) {}
        ~Destination() {}

      public:
        const char*    name();
        void           setVC(FEdest vc) { dest( (lane()<<2) | unsigned(vc) ); }
    };
  }
}

#endif
