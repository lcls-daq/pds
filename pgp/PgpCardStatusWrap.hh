/*
 * PgpCardStatusWrap.hh
 *
 *  Created on: Jan 12, 2016
 *      Author: jackp
 */

#ifndef PGPCARDSTATUSWRAP_HH_
#define PGPCARDSTATUSWRAP_HH_

#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/PgpStatus.hh"
#include "pgpcard/PgpCardMod.h"
#include "pgpcard/PgpCardStatus.h"
#include <time.h>

namespace Pds {

  namespace Pgp {

  class PgpCardStatusWrap : public PgpStatus {
      public:
        PgpCardStatusWrap(int f, unsigned d, Pgp* pgp) :
        	PgpStatus(f, d, pgp) {};
        virtual ~PgpCardStatusWrap() {};

      public:
        void              read();
        unsigned          checkPciNegotiatedBandwidth();
        int               allocateVC(unsigned vcm);
        int               allocateVC(unsigned vcm, unsigned l);
        int               resetPgpLane(unsigned);
        int               writeScratch(unsigned);
        void              print();

      protected:
        PgpCardStatus     status;
    };

  }

}

#endif /* PGPCARDSTATUSWRAP_HH_ */
