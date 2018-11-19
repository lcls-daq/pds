/*
 * Configurator.cc
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

#include "pds/pgp/Configurator.hh"
#include "pds/pgp/Destination.hh"
#include "pgpcard/PgpCardMod.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Pds {
  namespace Pgp {

     Configurator::Configurator(int f, unsigned d) : _fd(f), _debug(d) {
      _pgp = new Pds::Pgp::Pgp(false, _fd, true);
      _G3 = _pgp->G3Flag();
      checkPciNegotiatedBandwidth();
      printRes();
    }

    Configurator::Configurator(bool use_aes, int f, unsigned d, unsigned lane) : _fd(f), _debug(d) {
      _pgp = new Pds::Pgp::Pgp(use_aes, _fd, true, lane);
      _G3 = _pgp->G3Flag();
      checkPciNegotiatedBandwidth();
      printRes();
    }

    Configurator::~Configurator() {}

    void Configurator::print() {}

    long long int Configurator::timeDiff(timespec* end, timespec* start) {
      long long int diff;
      diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
      diff += end->tv_nsec;
      diff -= start->tv_nsec;
      return diff;
    }

    void Configurator::microSpin(unsigned m) {
      long long unsigned gap = 0;
      timespec start, now;
      clock_gettime(CLOCK_REALTIME, &start);
      while (gap < m) {
        clock_gettime(CLOCK_REALTIME, &now);
        gap = timeDiff(&now, &start) / 1000LL;
      }
    }

    void Configurator::printRes() {
      timespec now;
      clock_getres(CLOCK_REALTIME, &now);
      if (now.tv_sec == 0) {
        printf("Configurator sees time resolution in ns %u\n", (unsigned)now.tv_nsec);
      } else {
        printf("Configurator confused resolution %u:%u\n", (unsigned)now.tv_sec, (unsigned)now.tv_nsec);
      }
    }

    unsigned Configurator::checkPciNegotiatedBandwidth() {
      return _pgp->checkPciNegotiatedBandwidth();
    }

    unsigned Configurator::getCurrentFiducial(bool pr) {
      unsigned ret;
      timespec start, end;
      if (pr) {
        clock_gettime(CLOCK_REALTIME, &start);
      }
      ret = _pgp->getCurrentFiducial();
      if (pr) {
        clock_gettime(CLOCK_REALTIME, &end);
        printf("Configurator took %llu nsec to get fiducial\n", timeDiff(&end, &start));
      }
      return ret;
    }

    bool Configurator::evrEnabled(bool pf) {
      return _pgp->evrEnabled(pf);
    }

    int   Configurator::evrEnable(bool e) {
      int ret = 0;
      ret = _pgp->evrEnable(e);
      if (ret) {
        printf("Configurator failed to %sable evr!\n", e ? "en" : "dis");
      }
      return ret;
    }

    int   Configurator::fiducialTarget(unsigned t) {
      int ret = 0;
      ret = _pgp->setFiducialTarget(t);
      if (ret) {
        printf("Configurator failed to set lane %u to fiducial 0x%x\n", _pgp->portOffset(), t);
      }
      return ret;
    }

    int   Configurator::waitForFiducialMode(bool b) {
      int ret = 0;
      ret = _pgp->waitForFiducialMode(b);
      if (ret) {
        printf("Configurator failed to set fiducial mode for lane %u to %s\n",
            _pgp->portOffset(), b ? "true" : "false");
      }
      return ret;
    }

    int   Configurator::evrRunCode(unsigned c) {
      int ret = 0;
      ret = _pgp->evrRunCode(c);
      if (ret) {
        printf("Configurator failed to set evr run code %u for lane %u\n",
            c, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrRunDelay(unsigned d) {
      int ret = 0;
      ret = _pgp->evrRunDelay(d);
      if (ret) {
        printf("Configurator failed to set evr run delay %u for lane %u\n",
            d, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrDaqCode(unsigned c) {
      int ret = 0;
      ret = _pgp->evrDaqCode(c);
      if (ret) {
        printf("Configurator failed to set evr daq code %u for lane %u\n",
            c, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrDaqDelay(unsigned d) {
      int ret = 0;
      ret = _pgp->evrDaqDelay(d);
      if (ret) {
        printf("Configurator failed to set evr daq delay %u for lane %u\n",
            d, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrLaneEnable(bool e) {
      int ret = 0;
      if (_pgp) {
        ret = _pgp->evrLaneEnable(e);
        printf("Configurator  pgpEVR %sable lane completed\n", e ? "en" : "dis");
      } else {
    	  printf("Configurator failed to %sable lane because no pgp object\n", e ? "en" : "dis");
      }
      return ret;
    }

    int Configurator::evrEnableHdrChk(unsigned vc, bool e) {
      int ret;
      ret = _pgp->evrEnableHdrChk(vc, e);
      printf("Configurator::evrEnableHdrChk offset %d, vc %d, %s\n",
          _pgp->portOffset(), vc, e ? "true" : "false");
      return ret;
    }

    int Configurator::evrEnableHdrChkMask(unsigned vcm, bool e) {
      int ret;
      ret = _pgp->evrEnableHdrChkMask(vcm, e);
      printf("Configurator::evrEnableHdrChkMask offset %d, vcm %d, %s\n",
          _pgp->portOffset(), vcm, e ? "true" : "false");
      return ret;
    }

    int Configurator::allocateVC(unsigned vcm) {
      return _pgp->allocateVC(vcm);
    }

    int Configurator::allocateVC(unsigned vcm, unsigned lm) {
      return _pgp->allocateVC(vcm, lm);
    }

    int Configurator::cleanupEvr(unsigned vcm) {
      return _pgp->cleanupEvr(vcm);
    }

    int Configurator::cleanupEvr(unsigned vcm, unsigned lm) {
      return _pgp->cleanupEvr(vcm, lm);
    }

    int Configurator::writeScratch(unsigned s) {
      return(_pgp->writeScratch(s));
    }

    void Configurator::loadRunTimeConfigAdditions(char* name) {
      if (name && strlen(name)) {
        FILE* f;
        Destination _d;
        unsigned maxCount = 1024;
        char path[240];
        char* home = getenv("HOME");
        sprintf(path,"%s/%s",home, name);
        f = fopen (path, "r");
        if (!f) {
          char s[200];
          sprintf(s, "Could not open %s ", path);
          perror(s);
        } else {
          unsigned myi = 0;
          unsigned dest, addr, data;
          while (fscanf(f, "%x %x %x", &dest, &addr, &data) && !feof(f) && myi++ < maxCount) {
            _d.dest(dest);
            printf("\nRun time config: dest %s, addr 0x%x, data 0x%x ", _d.name(), addr, data);
            if(_pgp->writeRegister(&_d, addr, data)) {
              printf("\nConfigurator::loadRunTimeConfigAdditions failed on dest %u address 0x%x\n", dest, addr);
            }
          }
          if (!feof(f)) {
            perror("Error reading");
          }
//          printf("\nSpinning 200 microseconds\n");
//          microSpin(200);
        }
      }
    }

    void Configurator::dumpPgpCard() {
    	_pgp->printStatus();
    }

    unsigned ConfigSynch::_getOne() {
       Pds::Pgp::RegisterSlaveImportFrame* rsif;
       unsigned             count = 0;
       while ((count < 6) && (rsif = _cfgrt->pgp()->read(_size)) != 0) {
         if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting) {
           return Success;
         }
         count += 1;;
       }
       if (_printFlag) {
         printf("ConfigSynch::_getOne _pgp->read failed\n");
         if (count) printf(" after skipping %u\n", count);
       }
       return Failure;
     }

     bool ConfigSynch::take() {
       if (_depth == 0) {
         if (_getOne() == Failure) {
           return false;
         }
       } else {
         _depth -= 1;
       }
       _depthHisto[_depth] += 1;
       return true;
     }

     bool ConfigSynch::clear() {
       printf("ConfigSynch Depth Histo:\n");
       for(unsigned i=0; i<1000; i++) {
         if (_depthHisto[i] > 1) {
           printf("\t%u - %u\n", i, _depthHisto[i]);
         }
       }
       _printFlag = false | (_cfgrt->_debug&1);
       for (unsigned cnt=_depth; cnt<_length; cnt++) { _getOne(); }
       return true;
     }
  }
}
