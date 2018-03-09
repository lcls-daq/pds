/*
 * Epix100aDestination.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIX100ADESTINATION_HH_
#define EPIX100ADESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Epix100a {

    class Epix100aDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data=0, Registers=1, Oscilloscope=2, NumberOf=3};

        Epix100aDestination() {}
        ~Epix100aDestination() {}

      public:
        const char*         name();
    };
  }
}

#endif /* EPIX100ADESTINATION_HH_ */
