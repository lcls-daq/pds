
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
      pgpGetInfo(_pgp->fd(),&info);
      pgpGetPci(_pgp->fd(),&pciStatus);
      for (int x=0; x < G2_NUMBER_OF_LANES; x++) {
        pgpGetStatus(_pgp->fd(),x,&status[x]);
      }
    }

    unsigned AesDriverG2StatusWrap::checkPciNegotiatedBandwidth() {
      this->read();
      unsigned val = pciStatus.pciLanes;
      if (val < 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        esp = es + strlen(es);
      }
      return  val;
    }

    int AesDriverG2StatusWrap::resetPgpLane() {
      unsigned lane = _pgp->portOffset();
      unsigned tmp;
      ssize_t res = 0;
      res |= dmaReadRegister(_pgp->fd(), offsetof(PgpCardG2Regs, control), (unsigned*)&(tmp));
      tmp |= ((0x1000 << lane) & 0xF000);
      res |= dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, control), tmp);
      unsigned mask = 0xFFFFFFFF ^ ((0x1000 << lane) & 0xF000);
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, control), tmp);
      tmp |= ((0x100 << lane) & 0xF00);
      res |=  dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, control), tmp);
      mask = 0xFFFFFFFF ^ ((0x100 << lane) & 0xF00);
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, control), tmp);
      return res;
    }

    int AesDriverG2StatusWrap::writeScratch(unsigned s) {
      ssize_t res = 0;
      res |= dmaWriteRegister(_pgp->fd(), offsetof(PgpCardG2Regs, scratch), s);
      return res;
    }

    int AesDriverG2StatusWrap::allocateVC(unsigned vcm) {
      return allocateVC(vcm, 1);
    }

    int AesDriverG2StatusWrap::allocateVC(unsigned vcm, unsigned lm) {
      unsigned char maskBytes[DMA_MASK_SIZE];
      unsigned lane = _pgp->portOffset();
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
      int           x;
      this->read();
      printf("\nPGP Card Status:\n");

      printf("-------------- Card Info ------------------\n");
      printf("                 Type : 0x%.2x\n",info.type);
      printf("              Version : 0x%.8x\n",info.version);
      printf("               Serial : 0x%.16llx\n",(long long unsigned)(info.serial));
      printf("           BuildStamp : %s\n",info.buildStamp);
      printf("             LaneMask : 0x%.4x\n",info.laneMask);
      printf("            VcPerMask : 0x%.2x\n",info.vcPerMask);
      printf("              PgpRate : %i\n",info.pgpRate);
      printf("            PromPrgEn : %i\n",info.promPrgEn);

      printf("\n");
      printf("-------------- PCI Info -------------------\n");
      printf("           PciCommand : 0x%.4x\n",pciStatus.pciCommand);
      printf("            PciStatus : 0x%.4x\n",pciStatus.pciStatus);
      printf("          PciDCommand : 0x%.4x\n",pciStatus.pciDCommand);
      printf("           PciDStatus : 0x%.4x\n",pciStatus.pciDStatus);
      printf("          PciLCommand : 0x%.4x\n",pciStatus.pciLCommand);
      printf("           PciLStatus : 0x%.4x\n",pciStatus.pciLStatus);
      printf("         PciLinkState : 0x%x\n",pciStatus.pciLinkState);
      printf("          PciFunction : 0x%x\n",pciStatus.pciFunction);
      printf("            PciDevice : 0x%x\n",pciStatus.pciDevice);
      printf("               PciBus : 0x%.2x\n",pciStatus.pciBus);
      printf("             PciLanes : %i\n",pciStatus.pciLanes);

      for (x=0; x < G2_NUMBER_OF_LANES; x++) {
         if ( ((1 << x) & info.laneMask) == 0 ) continue;

         printf("\n");
         printf("-------------- Lane %i --------------------\n",x);
         printf("             LoopBack : %i\n",status[x].loopBack);
         printf("         LocLinkReady : %i\n",status[x].locLinkReady);
         printf("         RemLinkReady : %i\n",status[x].remLinkReady);
         printf("              RxReady : %i\n",status[x].rxReady);
         printf("              TxReady : %i\n",status[x].txReady);
         printf("              RxCount : %i\n",status[x].rxCount);
         printf("           CellErrCnt : %i\n",status[x].cellErrCnt);
         printf("          LinkDownCnt : %i\n",status[x].linkDownCnt);
         printf("           LinkErrCnt : %i\n",status[x].linkErrCnt);
         printf("              FifoErr : %i\n",status[x].fifoErr);
         printf("              RemData : 0x%.2x\n",status[x].remData);
         printf("        RemBuffStatus : 0x%.2x\n",status[x].remBuffStatus);
      }
    }
  }
}
