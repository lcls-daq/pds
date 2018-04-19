/*
 * AesDriverG3StatusWrap.cc
 *
 *  Created: Mon Feb  1 11:08:11 PST 2016
 *      Author: jackp
 */

#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/AesDriverG3StatusWrap.hh"
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

    void AesDriverG3StatusWrap::read() {
      pgpGetInfo(_pgp->fd(),&info);
      pgpGetPci(_pgp->fd(),&pciStatus);
      for (int x=0; x < G3_NUMBER_OF_LANES; x++) {
        pgpGetStatus(_pgp->fd(),x,&status[x]);
        pgpGetEvrStatus(_pgp->fd(),x,&evrStatus[x]);
        pgpGetEvrControl(_pgp->fd(),x,&evrControl[x]);
      }
    }

    unsigned AesDriverG3StatusWrap::checkPciNegotiatedBandwidth() {
      this->read();
      unsigned val = pciStatus.pciLanes;
      if (val != 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        esp = es + strlen(es);
      }
      return  val;
    }

    int AesDriverG3StatusWrap::resetSequenceCount(){
      unsigned lane = _pgp->portOffset();
      return pgpResetEvrCount(_pgp->fd(), lane);
    }

    int AesDriverG3StatusWrap::maskRunTrigger(bool b){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.laneRunMask = b ? 1 : 0;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }

    int AesDriverG3StatusWrap::resetPgpLane() {
      unsigned lane = _pgp->portOffset();
      unsigned tmp;
      ssize_t res = 0;
      res |= dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, pgpCardStat[0]), (unsigned*)&(tmp));
      tmp |= (0x1 << ((lane&0x7) + 16));
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, pgpCardStat[0]), tmp);
      unsigned mask = 0xFFFFFFFF ^(0x1 << ((lane&0x7) + 16));
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, pgpCardStat[0]), tmp);
      tmp  |= (0x1 << ((lane&0x7) + 8));
      res |=  dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, pgpCardStat[0]), tmp);
      mask = 0xFFFFFFFF ^ (0x1 << ((lane&0x7) + 8));
      tmp &= mask;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, pgpCardStat[0]), tmp);
      return res;
    }

    int AesDriverG3StatusWrap::writeScratch(unsigned s) {
      ssize_t res = 0;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, ScratchPad), s);
      return res;
    }

    unsigned AesDriverG3StatusWrap::getCurrentFiducial() {
      struct DmaRegisterData reg;
      reg.data = 0;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, evrFiducial);
      ioctl(_pgp->fd(),DMA_Read_Register,&reg);
      return ((unsigned)reg.data);
    }

    int AesDriverG3StatusWrap::setFiducialTarget(unsigned r){
      unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = r;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, fiducials[0]) + (lane*sizeof(unsigned));
      printf("AesDriverG3StatusWrap::setFiducialTarget 0x%x at 0x%x\n", reg.data, reg.address);
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);
    }
    int AesDriverG3StatusWrap::waitForFiducialMode(bool e){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.evrSyncSel = e ? 1 : 0;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrRunCode(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.runCode = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrRunDelay(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.runDelay = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrDaqCode(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.acceptCode = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrDaqDelay(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.acceptDelay = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrSetPulses(unsigned runcode, unsigned rundelay, unsigned daqcode, unsigned daqdelay){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.runCode = runcode;
      cntl.runDelay = rundelay;
      cntl.acceptCode = daqcode;
      cntl.acceptDelay = daqdelay;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrLaneEnable(bool e){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.evrSyncEn = e ? 1 : 0;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    int AesDriverG3StatusWrap::evrEnableHdrChk(unsigned vc, bool e) {
      return evrEnableHdrChkMask(1 << vc, e);
    }
    int AesDriverG3StatusWrap::evrEnableHdrChkMask(unsigned vcm, bool e) {
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      if (e) {
        cntl.headerMask |= vcm;
      } else {
        cntl.headerMask &= ~vcm;
      }
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
    }
    bool AesDriverG3StatusWrap::getLatestLaneStatus() {
      unsigned lane = _pgp->portOffset();
      return (evrStatus[lane].runStatus == 1);
    }

    bool AesDriverG3StatusWrap::evrEnabled(bool pf) {
      unsigned lane = _pgp->portOffset();
      this->read();
      bool enabled = evrControl[lane].evrEnable && evrStatus[lane].linkUp;
      if ((enabled == false) && pf) {
        printf("AesDriverG3StatusWrap: EVR not enabled, enable %s, ready %s\n",
            evrControl[lane].evrEnable ? "true" : "false", evrStatus[lane].linkUp ? "true" : "false");
        if (evrStatus[lane].linkUp != true) {
          sprintf(esp, "AesDriverG3StatusWrap: EVR not enabled\nMake sure MCC fiber is connected to pgpG3 card\n");
          printf("%s", esp);
          esp = es+strlen(es);
        }
      }
      return enabled;
    }

    int AesDriverG3StatusWrap::evrEnable(bool e) {
      unsigned lane = _pgp->portOffset();
      unsigned tmp;
      unsigned count=0;
      PgpEvrControl cntl;
      if (e) {
        if (evrEnabled(false)==false) {
          if (evrStatus[lane].linkUp) {
            pgpGetEvrControl(_pgp->fd(), lane, &cntl);
            cntl.evrEnable = 1;
            pgpSetEvrControl(_pgp->fd(), lane, &cntl);
          } else {
            while ((evrEnabled(false)==false) && (count++<3)){
              dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), &tmp);
              tmp |= 4;
              dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
              tmp &= 0xFFFFFFFB;
              dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
              usleep(40);
              tmp |= 2;
              dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
              tmp &= 0xFFFFFFFD;
              dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
              usleep(400);
              tmp |= 1;
              dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
              usleep(4000);
            }
          }
        }
      } else {
        pgpGetEvrControl(_pgp->fd(), lane, &cntl);
        cntl.evrEnable = 0;
        pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      }
      return 0;
    }

    int AesDriverG3StatusWrap::allocateVC(unsigned vcm) {
      return allocateVC(vcm, 1);
    }

    int AesDriverG3StatusWrap::allocateVC(unsigned vcm, unsigned lm) {
      unsigned char maskBytes[32];
      unsigned lane = _pgp->portOffset();
      dmaInitMaskBytes(maskBytes);
      for(unsigned i=0; i<G3_NUMBER_OF_LANES; i++) {
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

    int AesDriverG3StatusWrap::cleanupEvr(unsigned vcm) {
      return cleanupEvr(vcm, 1);
    }

    int AesDriverG3StatusWrap::cleanupEvr(unsigned vcm, unsigned lm) {
      unsigned offset = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      for(unsigned lane=0; lane<G3_NUMBER_OF_LANES; lane++) {
        if ((1<<lane) & (lm<<offset)) {
          res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
          cntl.evrSyncWord = 0;
          cntl.runCode = 0;
          cntl.acceptCode = 0;
          cntl.runDelay = 0;
          cntl.acceptDelay = 0;
          cntl.headerMask = 0;
          cntl.laneRunMask = 1;
          res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
        }
      }
      return res;
    }

    void AesDriverG3StatusWrap::print() {
      int           x;
      this->read();
      printf("\nPGP Card Status:\n");
      //      pgpGetInfo(_pgp->fd(),&info);
      //      pgpGetPci(_pgp->fd(),&pciStatus);
      //      unsigned low = (unsigned) (info.serial & 0xffffffff);
      //      unsigned high = (unsigned) ((info.serial>>32) & 0xffffffff);
      //      info.serial = (long long unsigned)high | ((long long unsigned)low<<32);

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

      printf("\n");
      printf("-------------- Lane Info --------------------\n             LoopBack : ");
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].loopBack, x==7?"\n         LocLinkReady : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].locLinkReady, x==7?"\n         RemLinkReady : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].remLinkReady, x==7?"\n              RxReady : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].rxReady, x==7?"\n              TxReady : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].txReady, x==7?"\n              RxCount : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].rxCount, x==7?"\n           CellErrCnt : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].cellErrCnt, x==7?"\n          LinkDownCnt : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].linkDownCnt, x==7?"\n           LinkErrCnt : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].linkErrCnt, x==7?"\n              FifoErr : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",status[x].fifoErr, x==7?"\n              RemData : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%.2x%s",status[x].remData, x==7?"\n        RemBuffStatus : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%.2x%s",status[x].remBuffStatus, x==7?"\n":" ");
      }
      printf("           LinkErrors : %i : MCC fiber errors\n",evrStatus[0].linkErrors);
      printf("               LinkUp : %i : MCC fiber status\n",evrStatus[0].linkUp);
      printf("            EvrEnable : %i : Global enable for the EVR firmware\n           RunStatus  : ",evrControl[0].evrEnable);     // Global flag
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrStatus[x].runStatus, x==7?" : 1 = Running, 0 = Stopped\n          EvrFiducial : ":" ");    // 1 = Running, 0 = Stopped
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%x%s", evrStatus[x].evrSeconds, x==7?"\n           RunCounter : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%x%s",evrStatus[x].runCounter, x==7?"\n        AcceptCounter : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrStatus[x].acceptCounter, x==7?"\n          LaneRunMask : ":" ");
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s", evrControl[x].laneRunMask, x==7?" : 0 = Run trigger enable\n            EvrLaneEn : ":" ");   // 0 = Run trigger enable
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].evrSyncEn, x==7?" : 1 = Start, 0 = Stop\n           EvrSyncSel : ":" ");     // 1 = Start, 0 = Stop
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].evrSyncSel, x==7?" : 0 = async, 1 = sync for start/stop\n           HeaderMask : ":" ");    // 0 = async, 1 = sync for start/stop
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%x%s",evrControl[x].headerMask, x==7?" : 1 = Enbl hdr checking, 1 bit per VC (4 bits)\n          EvrSyncWord : ":" ");    // 1 = Enable header data checking, one bit per VC (4 bits)
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("0x%x%s",evrControl[x].evrSyncWord, x==7?" : fiducial to transition start stop\n              RunCode : ":" ");   // fiducial to transition start stop
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].runCode, x==7?" : Run code\n             RunDelay : ":" ");       // Run code
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].runDelay, x==7?" : Run delay\n           AcceptCode : ":" ");      // Run delay
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].acceptCode, x==7?" : DAQ code\n          AcceptDelay : ":" ");    // Accept code
      }
      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
        printf("%i%s",evrControl[x].acceptDelay, x==7?" : DAQ delay\n":" ");   // Accept delay
      }
    }
  }
}
