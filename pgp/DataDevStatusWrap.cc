
#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/DataDevStatusWrap.hh"
#include <PgpDriver.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <termios.h>


namespace Pds {
  namespace Pgp {

    void DataDevStatusWrap::read() {
      axiVersionGet(_pgp->fd(),&info);
    }

    unsigned DataDevStatusWrap::checkPciNegotiatedBandwidth() {
      unsigned val = 4;
      if (val < 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        esp = es + strlen(es);
      }
      return  val;
    }

    int DataDevStatusWrap::writeScratch(unsigned s) {
      ssize_t res = 0;
      //res |= dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, scratch), s);
      return res;
    }

    int DataDevStatusWrap::allocateVC(unsigned vcm) {
      return allocateVC(vcm, 1<<_lane);
    }

    int DataDevStatusWrap::allocateVC(unsigned vcm, unsigned lm) {
      unsigned char maskBytes[DMA_MASK_SIZE];
      unsigned lane = _pgp->portOffset();
      dmaInitMaskBytes(maskBytes);
      for(unsigned i=0; i<G3_NUMBER_OF_LANES; i++) {
        if ((1<<i) & (lm<<lane)) {
          for(unsigned j=0; j<4; j++) {
            if ((1<<j) & vcm) {
              printf("allocatVC adding i %u, j %u, means %u\n", i, j, (i<<8)+j);
              dmaAddMaskBytes(maskBytes, (i<<8) + j);
            }
          }
        }
      }
      return dmaSetMaskBytes(_pgp->fd(), maskBytes);
    }

    void DataDevStatusWrap::print() {
      this->read();
      printf("\nDataDev Card Status:\n");

      printf("-------------- Card Info ------------------\n");
      printf("              Version : 0x%.8x\n",info.firmwareVersion);
      printf("               Serial : 0x%.16llx\n",(long long unsigned)(info.deviceId));
      printf("           BuildStamp : %s\n",info.buildString);
    }
  }
}
