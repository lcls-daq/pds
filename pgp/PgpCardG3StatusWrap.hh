/*
 * PgpCardG3StatusWrap.hh
 *
 *  Created on: Jan 12, 2016
 *      Author: jackp
 */

#ifndef PGPCARDG3STATUSWRAP_HH_
#define PGPCARDG3STATUSWRAP_HH_

#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include "pgpcard/PgpCardMod.h"
#include "pgpcard/PgpCardG3Status.h"
#include <time.h>

namespace Pds {

  namespace Pgp {

  class PgpCardG3StatusWrap : public PgpStatus {
      public:
        PgpCardG3StatusWrap(int f, unsigned d, Pgp* pgp) :
        	PgpStatus(f, d, pgp) {};
        virtual ~PgpCardG3StatusWrap() {};

      public:
        void              print(); 
        void              read(); 
        unsigned          checkPciNegotiatedBandwidth();
        unsigned          getCurrentFiducial();
        int               setFiducialTarget(unsigned);
        int               waitForFiducialMode(bool);
        int               evrRunCode(unsigned);
        int               evrRunDelay(unsigned);
        int               evrDaqCode(unsigned);
        int               evrDaqDelay(unsigned);
        int               evrSetPulses(unsigned, unsigned, unsigned, unsigned);
        int               evrLaneEnable(bool);
        int               evrEnableHdrChk(unsigned, bool);
        int               evrEnableHdrChkMask(unsigned, bool);
        bool              getLatestLaneStatus();
        bool              evrEnabled(bool);
        int               evrEnable(bool);
        int               allocateVC(unsigned vcm);
        int               allocateVC(unsigned vcm, unsigned l);
        int               resetSequenceCount();
        int               maskRunTrigger(bool b);
        int               resetPgpLane();
        int               writeScratch(unsigned);

      protected:
      PgpCardG3Status     status;
    };

  }

}

#endif /* PGPCARDG3STATUSWRAP_HH_ */
