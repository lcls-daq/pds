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
#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/PgpCardStatusWrap.hh"
#include "pds/pgp/PgpCardG3StatusWrap.hh"
#include "pds/pgp/AesDriverG2StatusWrap.hh"
#include "pds/pgp/AesDriverG3StatusWrap.hh"
#include "pgpcard/PgpCardMod.h"
#include <PgpDriver.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <new>

#define DMA_ERR_FIFO 0x01
#define DMA_ERR_LEN  0x02
#define DMA_ERR_MAX  0x04
#define DMA_ERR_BUS  0x08
#define PGP_ERR_EOFE 0x10

using namespace Pds::Pgp;

unsigned Pgp::_portOffset = 0;

static const unsigned SelectSleepTimeUSec=100000;

static void printError(unsigned error, unsigned dest)
{
  printf("Pgp::read error eofe(%s), fifo(%s), len(%s), bus%s\n",
         error & PGP_ERR_EOFE ? "true" : "false",
         error & DMA_ERR_FIFO ? "true" : "false",
         error & DMA_ERR_LEN ? "true" : "false",
         error & DMA_ERR_BUS ? "true" : "false");
  printf("\tpgpLane(%u), pgpVc(%u)\n", pgpGetLane(dest), pgpGetVc(dest));
}

Pgp::Pgp(bool use_aes, int f, bool pf, unsigned lane) : _fd(f), _useAesDriver(use_aes) {
  unsigned version = 0;
  if (pf) printf("Pgp::Pgp(fd(%d)), offset(%u)\n", f, _portOffset);
  if (!_useAesDriver) {
    _pt.model = sizeof(&_pt);
    _pt.size = sizeof(PgpCardTx);
    _pt.pgpLane = 0;
    _pt.pgpVc = 0;
  }
  for (unsigned i=0;i<4;i++) {_maskedHWerrorCount[i]= 0;}
  _maskHWerror = false;
  ::RegisterSlaveExportFrame::FileDescr(f, use_aes);
  for (int i=0; i<BufferWords; i++) _readBuffer[i] = i;
  _status = 0;
  // See if we can determine the type of card
  if (_useAesDriver) {
    _myG3Flag = true;
    dmaReadRegister(_fd, 0, (unsigned*)&(version) );
  } else {
    _myG3Flag = false;
    unsigned* ptr = (unsigned*)malloc(sizeof(PgpCardG3Status)); // the larger one
    int r = this->IoctlCommand(IOCTL_Read_Status, (long long unsigned)ptr);
    if (r<0) {
      perror("Unable to read the card status!\n");
    } else {
      version = ptr[0];
    }
    free(ptr);
  }
  _myG3Flag = ((version>>12) & 0xf) == 3;
  printf("Pgp::Pgp found card version 0x%x which I see is %sa G3 card\n", version, _myG3Flag ? "" : "NOT ");
  if (_myG3Flag) {
    if (use_aes) {
      _status = new AesDriverG3StatusWrap(_fd, 0, this, lane);
    } else {
      _status = new PgpCardG3StatusWrap(_fd, 0, this);
    }
  } else {
    if (use_aes) {
      _status = new AesDriverG2StatusWrap(_fd, 0, this);
    } else {
      _status = new PgpCardStatusWrap(_fd, 0, this);
    }
  }
}

Pgp::~Pgp() {}

bool Pgp::evrEnabled(bool pf) {
  return _status->evrEnabled(pf);
}
int Pgp::evrEnable(bool e) {
  return _status->evrEnable(e);
}
unsigned Pgp::Pgp::checkPciNegotiatedBandwidth() {
  return _status->checkPciNegotiatedBandwidth();
}
unsigned Pgp::Pgp::getCurrentFiducial() {
  return _status->getCurrentFiducial();
}
int      Pgp::Pgp::allocateVC(unsigned vcm) {
  return _status->allocateVC(vcm);
}
int      Pgp::Pgp::allocateVC(unsigned vcm, unsigned lm) {
  return _status->allocateVC(vcm, lm);
}
int      Pgp::Pgp::cleanupEvr(unsigned vcm) {
  return _status->cleanupEvr(vcm);
}
int      Pgp::Pgp::cleanupEvr(unsigned vcm, unsigned lm) {
  return _status->cleanupEvr(vcm, lm);
}
int      Pgp::Pgp::setFiducialTarget(unsigned t) {
  return _status->setFiducialTarget(t);
}
int      Pgp::Pgp:: waitForFiducialMode(bool t) {
  return _status->waitForFiducialMode(t);
};
int      Pgp::Pgp::evrRunCode(unsigned t) {
  return _status->evrRunCode(t);
};
int      Pgp::Pgp::evrRunDelay(unsigned d) {
  return _status->evrRunDelay(d);
};
int      Pgp::Pgp::evrDaqCode(unsigned c) {
  return _status->evrDaqCode(c);
};
int      Pgp::Pgp::evrDaqDelay(unsigned d) {
  return _status->evrDaqDelay(d);
};
int      Pgp::Pgp::evrLaneEnable(bool e) {
  return _status->evrLaneEnable(e);
};
int      Pgp::Pgp::evrEnableHdrChk(unsigned vc, bool e) {
  return _status->evrEnableHdrChk(vc, e);
}
int      Pgp::Pgp::evrEnableHdrChkMask(unsigned vcm, bool e) {
  return _status->evrEnableHdrChkMask(vcm, e);
}
bool     Pgp::Pgp::getLatestLaneStatus() {
  return _status->getLatestLaneStatus();
}
int      Pgp::Pgp::resetSequenceCount() {
  return _status->resetSequenceCount();
}
int      Pgp::Pgp::maskRunTrigger(bool b) {
  return _status->maskRunTrigger(b);
}
int      Pgp::Pgp::resetPgpLane() {
  return _status->resetPgpLane();
}
char*    Pgp::Pgp::errorString() {
  return _status->errorString();
}
void     Pgp::Pgp::errorStringAppend(char* s) {
  _status->errorStringAppend(s);
}
void     Pgp::Pgp::clearErrorString() {
  _status->clearErrorString();
}
int      Pgp::Pgp::writeScratch(unsigned s) {
  return _status->writeScratch(s);
}

void     Pgp::Pgp::printStatus() {
	_status->print();
	for(unsigned u=0; u<4; u++) {
		if (_maskedHWerrorCount[u]) {
			printf("Masked Hardware Errors for vc %u are %u\n\n", u, _maskedHWerrorCount[u]);
			_maskedHWerrorCount[u] = 0;
		}
	}
}

::RegisterSlaveImportFrame* Pgp::Pgp::read(unsigned size) {
  ::RegisterSlaveImportFrame* ret = (::RegisterSlaveImportFrame*)::Pgp::_readBuffer;
  int             sret = 0;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = SelectSleepTimeUSec;
  PgpCardRx       pgpCardRx;
  pgpCardRx.model   = sizeof(&pgpCardRx);
  pgpCardRx.maxSize = ::Pgp::BufferWords;
  pgpCardRx.data    = (__u32*)::Pgp::_readBuffer;
  fd_set          fds;
  FD_ZERO(&fds);
  FD_SET(::Pgp::_fd,&fds);
  int readRet = 0;
  bool   found  = false;
  if (_useAesDriver) {
    struct DmaReadData       pgpCardRx;
    pgpCardRx.flags   = 0;
    pgpCardRx.is32    = 0;
    pgpCardRx.dest    = 0;
    pgpCardRx.size = ::Pgp::BufferWords*sizeof(uint32_t);
    pgpCardRx.data    = (uint64_t)(&(::Pgp::_readBuffer));
    pgpCardRx.is32    = sizeof(&pgpCardRx)==4;
    while (found == false) {
      if ((sret = select(::Pgp::_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
        if ((readRet = ::read(::Pgp::_fd, &pgpCardRx, sizeof(struct DmaReadData))) >= 0) {
          if ((ret->waiting() == ::PgpRSBits::Waiting) || (ret->opcode() == ::PgpRSBits::read)) {
            //          printf("DMA Read Data\n");
            //          printf(" \tpgpCardRx.data\t0x%llx\n", (uint64_t)pgpCardRx.data);
            //          printf(" \tpgpCardRx.dest\t0x%x\n", pgpCardRx.dest);
            //          printf(" \tpgpCardRx.flags\t0x%x\n", pgpCardRx.flags);
            //          printf(" \tpgpCardRx.index\t0x%x\n", pgpCardRx.index);
            //          printf(" \tpgpCardRx.error\t0x%x\n", pgpCardRx.error);
            //          printf(" \tpgpCardRx.size\t0x%x\n", pgpCardRx.size);
            //          printf(" \tpgpCardRx.is32\t0x%x\n", pgpCardRx.is32);
            //          uint32_t* u = (uint32_t*)pgpCardRx.data;
            //          printf("data->0x%x 0x%x 0x%x\n", u[0], u[1], u[2]);
            found = true;
            if (pgpCardRx.error) {
              printError(pgpCardRx.error, pgpCardRx.dest);
              ret = 0;
            } else {
              if (readRet != int(size*sizeof(uint32_t))) {
                printf("Pgp::read read returned %u, we were looking for %u uint32s\n", (unsigned)(readRet/sizeof(uint32_t)), size);
                ret->print(readRet/sizeof(uint32_t));
                ret = 0;
              } else {
                bool hardwareFailure = false;
                uint32_t* u = (uint32_t*)ret;
                if (ret->failed((::LastBits*)(u+size-1))) {
                  if (_maskHWerror == false) {
                    printf("Pgp::read received HW failure\n");
                    ret->print();
                    hardwareFailure = true;
                  } else {
                    _maskedHWerrorCount[pgpCardRx.dest&3] += 1;
                  }
                }
                if (ret->timeout((::LastBits*)(u+size-1))) {
                  printf("Pgp::read received HW timed out\n");
                  ret->print();
                  hardwareFailure = true;
                }
                if (hardwareFailure) ret = 0;
              }
            }
          }
        } else {
          perror("Pgp::read() ERROR ! ");
          ret = 0;
          found = true;
        }
      } else {
        found = true;  // we might as well give up!
        ret = 0;
        if (sret < 0) {
          perror("Pgp::read(aes) select error: ");
          printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.dest>>2, pgpCardRx.dest&3);
        } else {
          printf("Pgp::read(aes) select timed out!\n");
        }
      }
    }
  } else {
    PgpCardRx       pgpCardRx;
    pgpCardRx.model   = sizeof(&pgpCardRx);
    pgpCardRx.maxSize = ::Pgp::BufferWords;
    pgpCardRx.data    = (__u32*)::Pgp::_readBuffer;
    while (found == false) {
      if ((sret = select(::Pgp::_fd+1,&fds,NULL,NULL,&timeout)) > 0) {
        if ((readRet = ::read(::Pgp::_fd, &pgpCardRx, sizeof(PgpCardRx))) >= 0) {
          if ((ret->waiting() == ::PgpRSBits::Waiting) || (ret->opcode() == ::PgpRSBits::read)) {
            found = true;
            if (pgpCardRx.eofe || pgpCardRx.fifoErr || pgpCardRx.lengthErr) {
              printf("Pgp::read error eofe(%u), fifoErr(%u), lengthErr(%u)\n",
                     pgpCardRx.eofe, pgpCardRx.fifoErr, pgpCardRx.lengthErr);
              printf("\tpgpLane(%u), pgpVc(%u)\n", pgpCardRx.pgpLane, pgpCardRx.pgpVc);
              ret = 0;
            } else {
              if (readRet != (int)size) {
                printf("Pgp::read read returned %u, we were looking for %u\n", readRet, size);
                ret->print(readRet);
                ret = 0;
              } else {
                bool hardwareFailure = false;
                uint32_t* u = (uint32_t*)ret;
                if (ret->failed((::LastBits*)(u+size-1))) {
                  if (_maskHWerror == false) {
                    printf("Pgp::read received HW failure\n");
                    ret->print();
                    hardwareFailure = true;
                  } else {
                    _maskedHWerrorCount[pgpCardRx.pgpVc] += 1;
                  }
                }
                if (ret->timeout((::LastBits*)(u+size-1))) {
                  printf("Pgp::read received HW timed out\n");
                  ret->print();
                  hardwareFailure = true;
                }
                if (hardwareFailure) ret = 0;
              }
            }
          }
        } else {
          perror("Pgp::read() ERROR ! ");
          ret = 0;
          found = true;
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
  }
  return ret;
}

unsigned Pgp::Pgp::stopPolling() {
  if (!_useAesDriver) {
    PgpCardTx* p = &_pt;

    p->cmd   = IOCTL_Clear_Polling;
    p->data  = 0;
    return(write(_fd, &p, sizeof(PgpCardTx)));
  } else {
    return 0;
  }
}

unsigned Pgp::Pgp::writeRegister(Destination* dest,
                                 unsigned addr,
                                 uint32_t data,
                                 bool printFlag,
                                 ::PgpRSBits::waitState w) {
  ::RegisterSlaveExportFrame rsef = 
    ::RegisterSlaveExportFrame(::PgpRSBits::write,
                                       dest,
                                       addr,
                                       0x6969,
                                       data,
                                       w);
  if (printFlag) rsef.print();
  return rsef.post(sizeof(rsef)/sizeof(uint32_t));
}

unsigned Pgp::Pgp::writeRegisterBlock(
                                      Destination* dest,
                                      unsigned addr,
                                      uint32_t* data,
                                      unsigned inSize,  // the size of the block to be exported in uint32 units
                                      ::PgpRSBits::waitState w,
                                      bool pf) {
  // the size of the export block plus the size of block to be exported minus the one that's already counted
  unsigned size = (sizeof(::RegisterSlaveExportFrame)/sizeof(uint32_t)) +  inSize -1;
  uint32_t myArray[size];
  ::RegisterSlaveExportFrame* rsef = 0;
  rsef = new (myArray) ::RegisterSlaveExportFrame(
                                                          ::PgpRSBits::write,
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

unsigned Pgp::Pgp::readRegister(
                                Destination* dest,
                                unsigned addr,
                                unsigned tid,
                                uint32_t* retp,
                                unsigned size,  // in uint32s
                                bool pf) {
  ::RegisterSlaveImportFrame* rsif;
    
  ::RegisterSlaveExportFrame  rsef = 
    ::RegisterSlaveExportFrame(::PgpRSBits::read,
                                       dest,
                                       addr,
                                       tid,
                                       size - 1,  // zero = one uint32_t, etc.
                                       ::PgpRSBits::Waiting);
  //      if (size>1) {
  //        printf("Pgp::readRegister size %u\n", size);
  //        rsef.print();
  //      }
  if (pf) rsef.print();
  if (rsef.post(sizeof(::RegisterSlaveExportFrame)/sizeof(uint32_t),pf) != Success) {
    printf("Pgp::readRegister failed, export frame follows.\n");
    rsef.print(0, sizeof(::RegisterSlaveExportFrame)/sizeof(uint32_t));
    return  Failure;
  }

  unsigned errorCount = 0;
  while (true) {
    rsif = this->read(size + 3);
    if (rsif == 0) {
      printf("Pgp::readRegister _pgp->read failed!\n");
      return Failure;
    }
    if (pf) rsif->print(size + 3);
    if (addr != rsif->addr()) {
      printf("Pgp::readRegister out of order response lane=%u, vc=%u, addr=0x%x(0x%x), tid=0x%x(0x%x), errorCount=%u\n",
             dest->lane(), dest->vc(), addr, rsif->addr(), tid, rsif->tid(), ++errorCount);
      rsif->print(size + 3);
      if (errorCount > 5) return Failure;
    } else {  // copy the data
      memcpy(retp, rsif->array(), size * sizeof(uint32_t));
      return Success;
    }
  }
}

int Pgp::Pgp::IoctlCommand(unsigned c, unsigned a) {
  if (!_useAesDriver) {
    PgpCardTx* p = &_pt;
    p->cmd   = c;
    p->data  = (__u32*) a;
    printf("IoctlCommand %u writing unsigned 0x%x\n", c, a);
    return(write(_fd, &_pt, sizeof(PgpCardTx)));
  } else {
    return -1;
  }
}

int Pgp::Pgp::IoctlCommand(unsigned c, long long unsigned a) {
  if (!_useAesDriver) {
    PgpCardTx* p = &_pt;
    p->cmd   = c;
    p->data  = (__u32*) a;
    //	printf("IoctlCommand %u writing long long 0x%llx\n", c, a);
    return(write(_fd, &_pt, sizeof(PgpCardTx)));
  } else {
    return -1;
  }
}

