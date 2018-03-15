/*
 * PgpStatus.hh
 *
 *  Created on: Jan 12, 2016
 *      Author: jackp
 */

#ifndef PGPSTATUS_HH_
#define PGPSTATUS_HH_

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include "pgpcard/PgpCardMod.h"
#include <time.h>
#include <stdlib.h>

namespace Pds {

  namespace Pgp {

  class Pgp;

  class PgpStatus {
      public:
        PgpStatus(int f, unsigned d, Pgp* pgp)   {
        	es = (char*)calloc(4096,1);
        	esp = es;
        	_fd = f;
        	_debug = d;
        	_pgp = pgp;
        	p = (PgpCardTx*)calloc(1, sizeof(PgpCardTx));
        	p->model = sizeof(p);
        };
        virtual ~PgpStatus() {
          free(p);
          free(es);
        };

      public:
        virtual void              print() = 0; 
        virtual unsigned          checkPciNegotiatedBandwidth() = 0;
        virtual unsigned          getCurrentFiducial() {return 0;}
        virtual int               setFiducialTarget(unsigned) {return 0;}
        virtual int               waitForFiducialMode(bool) {return 0;}
        virtual int               evrRunCode(unsigned) {return 0;}
        virtual int               evrRunDelay(unsigned) {return 0;}
        virtual int               evrDaqCode(unsigned) {return 0;}
        virtual int               evrDaqDelay(unsigned) {return 0;}
        virtual int               evrSetPulses(unsigned, unsigned, unsigned, unsigned) {return 0;}
        virtual int               evrLaneEnable(bool) {return 0;}
        virtual int               evrEnableHdrChk(unsigned, bool) {return 0;}
        virtual int               evrEnableHdrChkMask(unsigned, bool) {return 0;}
        virtual bool              getLatestLaneStatus() {return true;}
        virtual bool              evrEnabled(bool) {return true;}
        virtual int               evrEnable(bool) {return 0;}
        virtual int               allocateVC(unsigned vcm) {return 0;}
        virtual int               allocateVC(unsigned vcm, unsigned l) { return 0;}
        virtual int               resetSequenceCount() { return 0;}
        virtual int               maskRunTrigger(bool b) { return 0;}
        virtual int               resetPgpLane() {return 0;}
        virtual int               writeScratch(unsigned) = 0;
        char*                     errorString() {return es;}
        void                      errorStringAppend(char*);
        void                      clearErrorString();
        int                       fd() { return _fd; }
        Pds::Pgp::Pgp*            pgp() { return _pgp; }

      public:
        char*                     es;

      protected:
        virtual void                 read() = 0;
        char*                        esp;
        static int                   _fd;
        static unsigned              _debug;
        static Pds::Pgp::Pgp*        _pgp;
        static PgpCardTx*            p;
    };

  }

}

#endif /* PGPSTATUS_HH_ */
