/*
 * EpixDestination.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXDESTINATION_HH_
#define EPIXDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Epix {

    class EpixDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, NumberOf};

        EpixDestination() {}
        ~EpixDestination() {}

      public:
        const char*         name();
    };
  }
}

#endif /* DESTINATION_HH_ */
