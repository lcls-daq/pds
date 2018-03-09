/*
 * CspadDestination.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef CSPADDESTINATION_HH_
#define CSPADDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace CsPad {

    class CspadDestination : public Pds::Pgp::Destination {
      public:
        CspadDestination() {}
        ~CspadDestination() {}

        enum FEdest {CR=0, Q0=1, Q1=2, Q2=5, Q3=6, NumberOf=5};

      public:
        const char*         name();
//        CspadDestination::FEdest getDest(Pds::Pgp::RegisterSlaveImportFrame*);
    };
  }
}

#endif /* DESTINATION_HH_ */
