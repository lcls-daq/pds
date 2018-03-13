/*
 * ImpDestination.hh
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#ifndef IMPDESTINATION_HH_
#define IMPDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Imp {

    class ImpDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {InvalidDataVC=0, InvalidVC1=1, InvalidVC2=2, CommandVC=3, NumberOf=4};

        ImpDestination() {}
        ~ImpDestination() {}

      public:
        const char*         name();
    };
  }
}

#endif /* DESTINATION_HH_ */
