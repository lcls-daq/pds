/*
 * AesDriverG3StatusWrap.hh
 *
 *  Created on: Jan 12, 2016
 *      Author: jackp
 */

#ifndef AESDRIVERG3STATUSWRAP_HH_
#define AESDRIVERG3STATUSWRAP_HH_

#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include <PgpDriver.h>
#include <time.h>

namespace Pds {

  namespace Pgp {

  class AesDriverG3StatusWrap : public PgpStatus {
      public:
        AesDriverG3StatusWrap(int f, unsigned d, Pgp* pgp, unsigned lane=0) :
          PgpStatus(f, d, pgp), portOffset(pgp->portOffset()), fd(f), _lane(lane) {};
        virtual ~AesDriverG3StatusWrap() {};

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
        int               allocateVC(unsigned vcm, unsigned lm);
        int               cleanupEvr(unsigned vcm);
        int               cleanupEvr(unsigned vcm, unsigned lm);
        int               resetSequenceCount();
        int               maskRunTrigger(bool b);
        int               resetPgpLane();
        int               writeScratch(unsigned);

      protected:
        //  Next two members are to override static variables
        unsigned          portOffset;
        int               fd;
        PgpInfo           info;
        PciStatus         pciStatus;
        ::PgpStatus       status[G3_NUMBER_OF_LANES];
        PgpEvrStatus      evrStatus[G3_NUMBER_OF_LANES];
        PgpEvrControl     evrControl[G3_NUMBER_OF_LANES];
      private:
        unsigned          _lane;
    };

  }

}

#endif /* AESDRIVERG3STATUSWRAP_HH_ */
