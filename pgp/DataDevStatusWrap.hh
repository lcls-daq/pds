/*
 * DataDevStatusWrap.hh
 *
 *  Created on: Jun 27, 2025
 *      Author: ddamiani
 */

#ifndef DATADEVSTATUSWRAP_HH_
#define DATADEVSTATUSWRAP_HH_

#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include <PgpDriver.h>
#include <AxiVersion.h>
#include <time.h>

namespace Pds {

  namespace Pgp {

  class DataDevStatusWrap : public PgpStatus {
      public:
        DataDevStatusWrap(int f, unsigned d, Pgp* pgp, unsigned lane=0) :
        	PgpStatus(f, d, pgp), _lane(lane) {};
        virtual ~DataDevStatusWrap() {};

      public:
        void              read();
        unsigned          checkPciNegotiatedBandwidth();
        int               allocateVC(unsigned vcm);
        int               allocateVC(unsigned vcm, unsigned l);
        int               writeScratch(unsigned);
        void              print();

      protected:
        AxiVersion        info;

      private:
        unsigned          _lane;
    };

  }

}

#endif /* DATADEVSTATUSWRAP_HH_ */
