/*
 * AesDriverG2StatusWrap.hh
 *
 *  Created on: Mar 8, 2018
 *      Author: ddamiani
 */

#ifndef AESDRIVERG2STATUSWRAP_HH_
#define AESDRIVERG2STATUSWRAP_HH_

#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include <PgpDriver.h>
#include <time.h>

namespace Pds {

  namespace Pgp {

  class AesDriverG2StatusWrap : public PgpStatus {
      public:
        AesDriverG2StatusWrap(int f, unsigned d, Pgp* pgp) :
        	PgpStatus(f, d, pgp) {};
        virtual ~AesDriverG2StatusWrap() {};

      public:
        void              read();
        unsigned          checkPciNegotiatedBandwidth();
        int               allocateVC(unsigned vcm);
        int               allocateVC(unsigned vcm, unsigned l);
        int               resetPgpLane();
        int               writeScratch(unsigned);
        void              print();

      protected:
        PgpInfo           info;
        PciStatus         pciStatus;
        ::PgpStatus       status[G2_NUMBER_OF_LANES];
    };

  }

}

#endif /* AESDRIVERG2STATUSWRAP_HH_ */
