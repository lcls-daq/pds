/*
 * Epix10kaDestination.hh
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#ifndef EPIX10KADESTINATION_HH_
#define EPIX10KADESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Epix10ka {

    class Epix10kaDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Registers=0, Data=1, Oscilloscope=2, NumberOf=3};

        Epix10kaDestination() {}
        ~Epix10kaDestination() {}

      public:
        const char*         name();
    };
  }
}

#endif /** EPIX10KADESTINATION_HH_ */
