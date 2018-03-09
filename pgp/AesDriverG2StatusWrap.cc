
#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/AesDriverG2StatusWrap.hh"
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

    void AesDriverG2StatusWrap::read() {
      //unsigned tmp;
      //struct DmaRegisterData reg;
      //reg.data = 0;
      memset(&status,0,sizeof(PgpCardStatus));
      dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, version), (unsigned*)&(status.Version) );
      dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, scratch), (unsigned*)&(status.ScratchPad) );
      //dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, SerialNumber[1]), (unsigned*)&(status.SerialNumber[1]) );
      //for(unsigned i=0; i<64; i++) {
      //  reg.address = (unsigned)offsetof(PgpCardG3Regs, BuildStamp[0]) + (i*sizeof(unsigned));
      //  ioctl(_pgp->fd(),DMA_Read_Register,&reg);
      //  status.BuildStamp[i] = reg.data;
      //}
      //dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, cardRstStat), &tmp );
      //status.CountReset = tmp & 1;
      //status.CardReset = (tmp>>1) & 1;

    }

    unsigned AesDriverG2StatusWrap::checkPciNegotiatedBandwidth() {
      unsigned tmp, tmp2;
      unsigned offset = (unsigned)offsetof(PgpCardG2Regs, pciStat2);
      dmaReadRegister(_pgp->fd(), offset, (unsigned*)&(tmp) );
      tmp2 = (tmp >> 4) & 0x3f;
      if (tmp2 != 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G2 card\n", tmp2);
        printf("%s\tregister was %u, offset %u\n", esp, tmp, offset);
        esp = es + strlen(es);
      }
      return  tmp2;
    }

    int AesDriverG2StatusWrap::resetPgpLane(unsigned lane) {
      unsigned tmp;
      ssize_t res = 0;
      res |= dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, control), (unsigned*)&(tmp));
      tmp |= ((0x1000 << lane) & 0xF000);
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, control), tmp);
      unsigned mask = 0xFFFFFFFF ^ ((0x1000 << lane) & 0xF000);
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, control), tmp);
      tmp |= ((0x100 << lane) & 0xF00);
      res |=  dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, control), tmp);
      mask = 0xFFFFFFFF ^ ((0x100 << lane) & 0xF00);
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, control), tmp);
      return res;
    }

    int AesDriverG2StatusWrap::writeScratch(unsigned s) {
      ssize_t res = 0;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG2Regs, scratch), s);
      return res;
    }

    int AesDriverG2StatusWrap::allocateVC(unsigned vcm) {
      return allocateVC(vcm, 1);
    }

    int AesDriverG2StatusWrap::allocateVC(unsigned vcm, unsigned lm) {
      unsigned char maskBytes[32];
      unsigned lane = pgp()->portOffset();
      dmaInitMaskBytes(maskBytes);
      for(unsigned i=0; i<4; i++) {
        if ((1<<i) & (lm<<lane)) {
          for(unsigned j=0; j<4; j++) {
            if ((1<<j) & vcm) {
              printf("allocatVC adding i %u, j %u, means %u\n", i, j, (i<<2)+j);
              dmaAddMaskBytes(maskBytes, (i<<2) + j);
            }
          }
        }
      }
      return dmaSetMaskBytes(_pgp->fd(), maskBytes);
    }

    void AesDriverG2StatusWrap::print() {

    }
  }
}
