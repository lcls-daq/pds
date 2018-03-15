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
      /*unsigned tmp, tmp2;
      unsigned offset = (unsigned)offsetof(PgpCardG3Regs, pciStat[2]);
      dmaReadRegister(_pgp->fd(), offset, (unsigned*)&(tmp) );
      tmp2 = (tmp >> 4) & 0x3f;*/
      unsigned val = pciStatus.pciLanes;
      this->read();
      if (val != 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        esp = es + strlen(es);
      }
      return  val;
    }

    int AesDriverG3StatusWrap::resetSequenceCount(){
      unsigned lane = _pgp->portOffset();
      return pgpResetEvrCount(_pgp->fd(), lane);
      /*unsigned tmp;
      ssize_t res = 0;
      res |= dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), (unsigned*)&(tmp) );
      tmp |= (mask<<8);
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), tmp);
      tmp &= 0xFFFF00FF;
      res |= dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), tmp);
      return res;*/
    }

    int AesDriverG3StatusWrap::maskRunTrigger(bool b){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.laneRunMask = b ? 1 : 0;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
      /*unsigned tmp;
      ssize_t res = 0;
      res |= dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), (unsigned*)&(tmp) );
      if(b) {
        tmp |= mask << 16;
      } else {
        tmp &= ~(mask<<16);
      }
      dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), tmp);
      return res;*/
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
      /*unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = 0;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]);
      ioctl(_pgp->fd(),DMA_Read_Register,&reg);
      if (e) {
        reg.data |= 1 << (8 + lane);
      } else {
        reg.data &= ~(1 << (8 + lane));
      }
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);*/
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
      /*unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = r;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, runCode[0]) + lane*sizeof(unsigned);
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);*/
    }
    int AesDriverG3StatusWrap::evrRunDelay(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.runDelay = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
      /*unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = r;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, runDelay[0]) + lane*sizeof(unsigned);
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);*/
    }
    int AesDriverG3StatusWrap::evrDaqCode(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.acceptCode = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
      /*unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = r;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, acceptCode[0]) + lane*sizeof(unsigned);
      printf("AesDriverG3StatusWrap::evrDaqCode %u at 0x%x\n", reg.data, reg.address);
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);*/
    }
    int AesDriverG3StatusWrap::evrDaqDelay(unsigned r){
      unsigned lane = _pgp->portOffset();
      ssize_t res = 0;
      PgpEvrControl cntl;
      res |= pgpGetEvrControl(_pgp->fd(), lane, &cntl);
      cntl.acceptDelay = r;
      res |= pgpSetEvrControl(_pgp->fd(), lane, &cntl);
      return res;
      /*unsigned lane = _pgp->portOffset();
      struct DmaRegisterData reg;
      reg.data = r;
      reg.address = (unsigned)offsetof(PgpCardG3Regs, acceptDelay[0]) + lane*sizeof(unsigned);
      printf("AesDriverG3StatusWrap::evrDaqDelay %u at 0x%x\n", reg.data, reg.address);
      ioctl(_pgp->fd(),DMA_Write_Register,&reg);
      return (0);*/
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
      /*unsigned tmp;
      unsigned status;
      dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), &tmp);
      status = (tmp >> 24) & 0xff;
      return ((status >> _pgp->portOffset())&1);*/
      unsigned lane = _pgp->portOffset();
      return (evrStatus[lane].runStatus == 1);
    }

    bool AesDriverG3StatusWrap::evrEnabled(bool pf) {
      unsigned lane = _pgp->portOffset();
      this->read();
      bool enabled = evrControl[lane].evrEnable && evrStatus[lane].linkUp;
      /*unsigned tmp;
      dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), &tmp);
      bool enable = (tmp & 1) == 1;
      dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), &tmp);
      bool ready = ((tmp >> 4) & 1) == 1;
      bool enabled = enable && ready;*/
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
        /*dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[0]), &tmp);
        if((tmp>>4)&1) {
          dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), &tmp);
          tmp |= 1;
          dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
        } else {
          while (count++<3){
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
        }*/
      } else {
        pgpGetEvrControl(_pgp->fd(), lane, &cntl);
        cntl.evrEnable = 0;
        pgpSetEvrControl(_pgp->fd(), lane, &cntl);
        //dmaReadRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), &tmp);
        //tmp &= 0xFFFFFFFE;
        //dmaWriteRegister(_pgp->fd(), (unsigned)offsetof(PgpCardG3Regs, evrCardStat[1]), tmp);
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
      for(unsigned i=0; i<8; i++) {
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

      for (x=0; x < G3_NUMBER_OF_LANES; x++) {
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
         printf("           LinkErrors : %i\n",evrStatus[x].linkErrors);
         printf("               LinkUp : %i\n",evrStatus[x].linkUp);
         printf("            RunStatus : %i 1 = Running, 0 = Stopped\n",evrStatus[x].runStatus);    // 1 = Running, 0 = Stopped
         printf("          EvrFiducial : 0x%x\n",evrStatus[x].evrSeconds);
         printf("           RunCounter : 0x%x\n",evrStatus[x].runCounter);
         printf("        AcceptCounter : %i\n",evrStatus[x].acceptCounter);
         printf("            EvrEnable : %i Global flag\n",evrControl[x].evrEnable);     // Global flag
         printf("          LaneRunMask : %i 0 = Run trigger enable\n",evrControl[x].laneRunMask);   // 1 = Run trigger enable
         printf("            EvrLaneEn : %i 1 = Start, 0 = Stop\n",evrControl[x].evrSyncEn);     // 1 = Start, 0 = Stop
         printf("           EvrSyncSel : %i 0 = async, 1 = sync for start/stop\n",evrControl[x].evrSyncSel);    // 0 = async, 1 = sync for start/stop
         printf("           HeaderMask : 0x%x 1 = Enable header data checking, one bit per VC (4 bits)\n",evrControl[x].headerMask);    // 1 = Enable header data checking, one bit per VC (4 bits)
         printf("          EvrSyncWord : 0x%x fiducial to transition start stop\n",evrControl[x].evrSyncWord);   // fiducial to transition start stop
         printf("              RunCode : %i Run code\n",evrControl[x].runCode);       // Run code
         printf("             RunDelay : %i Run delay\n",evrControl[x].runDelay);      // Run delay
         printf("           AcceptCode : %i DAQ code\n",evrControl[x].acceptCode);    // Accept code
         printf("          AcceptDelay : %i DAQ delay\n",evrControl[x].acceptDelay);   // Accept delay
      }

//      __u64 SerialNumber = status.SerialNumber[0];
//      SerialNumber = SerialNumber << 32;
//      SerialNumber |= status.SerialNumber[1];
//
//      printf("           Version: 0x%x\n", status.Version);
//      printf("      SerialNumber: 0x%llx\n", SerialNumber);
//      printf("        BuildStamp: %s\n",(char *) (status.BuildStamp) );
//      printf("        CountReset: 0x%x\n", status.CountReset);
//      printf("         CardReset: 0x%x\n", status.CardReset);
    }
  }
}
