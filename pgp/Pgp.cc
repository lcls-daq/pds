/*
 * Pgp.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/pgp/Pgp.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/Destination.hh"
#include "PgpCardMod.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <new>

namespace Pds {

  namespace Pgp {

    Pgp::Pgp(int f) : _fd(f) {}

    Pgp::~Pgp() {}

    Pds::Pgp::RegisterSlaveImportFrame* Pgp::read(unsigned size) {
      Pds::Pgp::RegisterSlaveImportFrame* ret = (Pds::Pgp::RegisterSlaveImportFrame*)_readBuffer;
      int             sret = 0;
      struct timeval  timeout;
      timeout.tv_sec  = 0;
      timeout.tv_usec = SelectSleepTimeUSec;
      PgpCardRx       pgpCardRx;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = BufferWords;
      pgpCardRx.data    = (__u32*)_readBuffer;
      fd_set          fds;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);
      unsigned readRet = 0;
      bool   found  = false;
      while (found == false) {
        if ((sret = select(_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
          readRet = ::read(_fd, &pgpCardRx, sizeof(PgpCardRx));
          if ((ret->waiting() == Pds::Pgp::PgpRSBits::Waiting) || (ret->opcode() == Pds::Pgp::PgpRSBits::read)) {
            found = true;
            if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
              printf("Pgp::read error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
                  pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
              printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
              ret = 0;
            } else {
              if (readRet != size) {
                printf("Pgp::read read returned %u, we were looking for %u\n", readRet, size);
                ret->print(readRet);
                ret = 0;
              } else {
                bool hardwareFailure = false;
                uint32_t* u = (uint32_t*)ret;
                if (ret->failed((Pds::Pgp::LastBits*)(u+size-1))) {
                  printf("Pgp::read received HW failure\n");
                  ret->print();
                  hardwareFailure = true;
                }
                if (ret->timeout((Pds::Pgp::LastBits*)(u+size-1))) {
                  printf("Pgp::read received HW timed out\n");
                  ret->print();
                  hardwareFailure = true;
                }
                if (hardwareFailure) ret = 0;
              }
            }
          }
        } else {
          found = true;  // we might as well give up!
          ret = 0;
          if (sret < 0) {
            perror("Pgp::read select error: ");
            printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
          } else {
            printf("Pgp::read select timed out!\n");
          }
        }
      }
      return ret;
    }

    unsigned Pgp::writeRegister(
        Destination* dest,
        unsigned addr,
        uint32_t data,
        bool printFlag,
        Pds::Pgp::PgpRSBits::waitState w) {
      Pds::Pgp::RegisterSlaveExportFrame rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::PgpRSBits::write,
              dest,
              addr,
              0x6969,
              data,
              w);
      if (printFlag) rsef.print();
      return rsef.post(sizeof(rsef)/sizeof(uint32_t));
    }

    unsigned Pgp::writeRegisterBlock(
        Destination* dest,
        unsigned addr,
        uint32_t* data,
        unsigned inSize,
        Pds::Pgp::PgpRSBits::waitState w,
        bool pf) {
      // the size of the export block plus the size of block to be exported minus the one that's already counted
      unsigned size = (sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) +  inSize -1;
      uint32_t myArray[size];
      Pds::Pgp::RegisterSlaveExportFrame* rsef = 0;
      rsef = new (myArray) Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
              Pds::Pgp::PgpRSBits::write,
              dest,
              addr,
              0x6970,
              0,
              w);
      memcpy(rsef->array(), data, inSize*sizeof(uint32_t));
      rsef->array()[inSize] = 0;
      if (pf) rsef->print(0, size);
      return rsef->post(size);
    }

    unsigned Pgp::readRegister(
        Destination* dest,
        unsigned addr,
        unsigned tid,
        uint32_t* retp,
        unsigned size,
        bool pf) {
      Pds::Pgp::RegisterSlaveImportFrame* rsif;
      Pds::Pgp::RegisterSlaveExportFrame  rsef = Pds::Pgp::RegisterSlaveExportFrame::RegisterSlaveExportFrame(
          Pds::Pgp::PgpRSBits::read,
          dest,
          addr,
          tid,
          size - 1,  // zero = one uint32_t, etc.
          Pds::Pgp::PgpRSBits::Waiting);
//      if (size>1) {
//        printf("Pgp::readRegister size %u\n", size);
//        rsef.print();
//      }
      if (pf) rsef.print();
      if (rsef.post(sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t)) != Success) {
        printf("Pgp::readRegister failed, export frame follows.\n");
        rsef.print(0, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
        return  Failure;
      }
      unsigned errorCount = 0;
      while (true) {
        rsif = this->read(size + 3);
        if (pf) rsif->print(size + 3);
        if (rsif == 0) {
          printf("Pgp::readRegister _pgp->read failed!\n");
          return Failure;
        }
        if (addr != rsif->addr()) {
          printf("Pds::Pgp::readRegister out of order response lane=%u, vc=%u, addr=0x%x, tid=%u, errorCount=%u\n",
              dest->lane(), dest->vc(), addr, tid, ++errorCount);
          rsif->print(size + 3);
          if (errorCount > 5) return Failure;
        } else {  // copy the data
          memcpy(retp, rsif->array(), size * sizeof(uint32_t));
          return Success;
        }
      }
    }

  }

}