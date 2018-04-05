/*
 * PgpCardG3StatusWrap.cc
 *
 *  Created: Mon Feb  1 11:08:11 PST 2016
 *      Author: jackp
 */

#include "pds/pgp/PgpStatus.hh"
#include "pds/pgp/PgpCardG3StatusWrap.hh"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <termios.h>


namespace Pds {
  namespace Pgp {

  void PgpCardG3StatusWrap::read() {
      memset(&status,0,sizeof(PgpCardG3Status));
      p->cmd   = IOCTL_Read_Status;
      p->data  = (__u32*)(&status);
      write(_fd, p, sizeof(PgpCardTx));
    }

    unsigned PgpCardG3StatusWrap::checkPciNegotiatedBandwidth() {
      this->read();
      unsigned val = (status.PciLStatus >> 4)  & 0x3f;
      if (val != 4) {
        sprintf(esp, "Negotiated bandwidth too low, %u\n Try reinstalling or replacing PGP G3 card\n", val);
        printf("%s", esp);
        esp = es + strlen(es);
      }
      return  val;
    }

    unsigned PgpCardG3StatusWrap::getCurrentFiducial() {
      this->read();
      return (status.EvrFiducial);
    }

    int PgpCardG3StatusWrap::setFiducialTarget(unsigned r) {
      unsigned arg = (_pgp->portOffset()<<28) | (r & 0x1ffff);
      return _pgp->IoctlCommand(IOCTL_Evr_Fiducial, arg);
    }

    int PgpCardG3StatusWrap::waitForFiducialMode(bool e) {
      unsigned arg = (1u << _pgp->portOffset());
      return _pgp->IoctlCommand(e ? IOCTL_Evr_LaneModeFiducial : IOCTL_Evr_LaneModeNoFiducial, arg);
    }

    int PgpCardG3StatusWrap::evrRunCode(unsigned r) {
      unsigned arg = (_pgp->portOffset() << 28) | (r & 0xff);
      return _pgp->IoctlCommand(IOCTL_Evr_RunCode, arg);
    }

    int PgpCardG3StatusWrap::evrRunDelay(unsigned r) {
      unsigned arg = (_pgp->portOffset() << 28) | (r & 0xfffffff);
      return _pgp->IoctlCommand(IOCTL_Evr_RunDelay, arg);
    }

    int PgpCardG3StatusWrap::evrDaqCode(unsigned r) {
      unsigned arg = (_pgp->portOffset() << 28) | (r & 0xff);
      return _pgp->IoctlCommand(IOCTL_Evr_AcceptCode, arg);
    }

    int PgpCardG3StatusWrap::evrDaqDelay(unsigned r) {
      unsigned arg = (_pgp->portOffset() << 28) | (r & 0xfffffff);
      return _pgp->IoctlCommand(IOCTL_Evr_AcceptDelay, arg);
    }

    int PgpCardG3StatusWrap::evrSetPulses(unsigned runcode, unsigned rundelay, unsigned daqcode, unsigned daqdelay) {
      int ret = 0;
      ret |= evrRunCode(runcode);
      ret |= evrRunDelay(rundelay);
      ret |= evrDaqCode(daqcode);
      ret |= evrDaqDelay(daqdelay);
      return ret;
    }

    int PgpCardG3StatusWrap::evrLaneEnable(bool e) {
      unsigned mask = 1 << _pgp->portOffset();
      if (e) {
        return _pgp->IoctlCommand( IOCTL_Evr_LaneEnable, mask);
      } else {
        return _pgp->IoctlCommand( IOCTL_Evr_LaneDisable, mask);
      }  
    }

    int PgpCardG3StatusWrap::evrEnableHdrChk(unsigned vc, bool e) {
      unsigned arg = (_pgp->portOffset()<<28) | (vc<<24) | (e ? 1 : 0);
      return _pgp->IoctlCommand( IOCTL_Evr_En_Hdr_Check, arg);
    }

    int PgpCardG3StatusWrap::evrEnableHdrChkMask(unsigned vcm, bool e) {
      int ret = 0;
      for (int i=0; i<4; i++) {
        if (vcm>>i & 0x1) {
          ret |= evrEnableHdrChk(i, e);
        }
      }
      return ret;
    }

    bool PgpCardG3StatusWrap::getLatestLaneStatus() {
      return ((status.EvrLaneStatus >> pgp()->portOffset())&1);
    }

    bool PgpCardG3StatusWrap::evrEnabled(bool pf) {
      this->read();
      bool enabled = status.EvrEnable && status.EvrReady;
      if ((enabled == false) && pf) {
        printf("PgpCardG3StatusWrap: EVR not enabled, enable %s, ready %s\n",
            status.EvrEnable ? "true" : "false", status.EvrReady ? "true" : "false");
        if (status.EvrReady == false) {
          sprintf(esp, "PgpCardG3StatusWrap: EVR not enabled\nMake sure MCC fiber is connected to pgpG3 card\n");
          printf("%s", esp);
          esp = es+strlen(es);
        }
      }
      return enabled;
    }

    int PgpCardG3StatusWrap::evrEnable(bool e) {
      int ret = 0;
      unsigned z = 0;
      unsigned count = 0;
      if (e) {
        while ((evrEnabled(false)==false) && (count++ < 3) && (ret==0)) {
          printf("Attempting to enable evr %u\n", count);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Set_PLL_RST, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Clr_PLL_RST, z);
          usleep(40);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Set_Reset, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Clr_Reset, z);
          usleep(400);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Enable, z);
          usleep(4000);
        }
      } else {
        ret = _pgp->IoctlCommand( IOCTL_Evr_Disable, z);
      }
      if (ret || (count >= 3)) {
        printf("Failed to %sable evr!\n", e ? "en" : "dis");
      }
      return ret || (count >= 3);
    }

    int PgpCardG3StatusWrap::allocateVC(unsigned vcm) {
      return allocateVC(vcm, 1);
    }

    int PgpCardG3StatusWrap::allocateVC(unsigned vcm, unsigned lm) {
      int ret = 0;
      bool found = false;
      unsigned arg = (vcm<<8);
      unsigned mask = (lm << _pgp->portOffset());
      unsigned extra = 0;
      // Find the first lane in the mask and find number of additional ports to add
      for (int i=0; i<G3_NUMBER_OF_LANES; i++) {
        if (mask & (1<<i)) {
          if (found) {
            extra++;
          } else {
            arg |= i;
            found = true;
          }
        } else {
          if (found) break;
        }
      }
      if (extra) {
        ret |= _pgp->IoctlCommand(IOCTL_Add_More_Ports, extra);
      }
      ret |= _pgp->IoctlCommand(IOCTL_Set_VC_Mask, arg);
      return ret;
    }

    int PgpCardG3StatusWrap::cleanupEvr(unsigned vcm) {
      return cleanupEvr(vcm, 1);
    }

    int PgpCardG3StatusWrap::cleanupEvr(unsigned vcm, unsigned lm) {
      unsigned offset = _pgp->portOffset();
      unsigned arg = ((lm<<offset)<<24) | 1;
      int ret = _pgp->IoctlCommand(IOCTL_Evr_RunMask, arg);
      for(unsigned lane=0; lane<G3_NUMBER_OF_LANES; lane++) {
        if ((1<<lane) & (lm<<offset)) {
          for (int vc=0; vc<4; vc++) {
            if (vcm>>vc & 0x1) {
              arg = (lane<<28) | (vc<<24);
              ret |= _pgp->IoctlCommand( IOCTL_Evr_En_Hdr_Check, arg);
            }
          }
        }
      }
      return ret;
    }

    int PgpCardG3StatusWrap::resetSequenceCount() {
      unsigned mask = 1 << _pgp->portOffset();
      return _pgp->IoctlCommand(IOCTL_ClearFrameCounter, mask);
    }

    int PgpCardG3StatusWrap::maskRunTrigger(bool b) {
      unsigned mask = 1 << _pgp->portOffset();
      unsigned flag = b ? 1 : 0;
      return _pgp->IoctlCommand(IOCTL_Evr_RunMask, (unsigned)((mask<<24) | flag));
    }

    int PgpCardG3StatusWrap::resetPgpLane() {
      unsigned lane = _pgp->portOffset();
      int ret = 0;
      ret |= _pgp->IoctlCommand(IOCTL_Set_Tx_Reset, lane);
      ret |= _pgp->IoctlCommand(IOCTL_Clr_Tx_Reset, lane);
      ret |= _pgp->IoctlCommand(IOCTL_Set_Rx_Reset, lane);
      ret |= _pgp->IoctlCommand(IOCTL_Clr_Rx_Reset, lane);
      return ret;
    }

    int PgpCardG3StatusWrap::writeScratch(unsigned s) {
      return _pgp->IoctlCommand(IOCTL_Write_Scratch, s);
    }

    void PgpCardG3StatusWrap::print() {
  	  int           x;
  	  int           y;
      this->read();
      printf("\nPGP Card Status:\n");

      __u64 SerialNumber = status.SerialNumber[0];
      SerialNumber = SerialNumber << 32;
      SerialNumber |= status.SerialNumber[1];

      printf("           Version: 0x%x\n", status.Version);
      printf("      SerialNumber: 0x%llx\n", SerialNumber);
      printf("        BuildStamp: %s\n",(char *) (status.BuildStamp) );
      printf("        CountReset: 0x%x\n", status.CountReset);
      printf("         CardReset: 0x%x\n", status.CardReset);
      printf("        ScratchPad: 0x%x\n\n", status.ScratchPad);

      printf("          PciCommand: 0x%x\n", status.PciCommand);
      printf("           PciStatus: 0x%x\n", status.PciStatus);
      printf("         PciDCommand: 0x%x\n", status.PciDCommand);
      printf("          PciDStatus: 0x%x\n", status.PciDStatus);
      printf("         PciLCommand: 0x%x\n", status.PciLCommand);
      printf("          PciLStatus: 0x%x\n", status.PciLStatus);
      printf("Negotiated Link Width:  %d", (status.PciLStatus >> 4) & 0x3f);
      printf("        PciLinkState: 0x%x\n", status.PciLinkState);
      printf("         PciFunction: 0x%x\n", status.PciFunction);
      printf("           PciDevice: 0x%x\n", status.PciDevice);
      printf("              PciBus: 0x%x\n", status.PciBus);
      printf("         PciBaseAddr: 0x%x\n", status.PciBaseHdwr);
      printf("       PciBaseLength: 0x%x\n\n", status.PciBaseLen);

      printf("            PgpRate(Gbps): %f\n",  ((double)status.PpgRate)*1.0E-3);
      printf("         PgpLoopBack[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpLoopBack[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("          PgpTxReset[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpTxReset[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("          PgpRxReset[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpRxReset[x]);
        if(x!=7) printf(", "); else printf("\n");
      }


      printf(" PgpTxCommonPllReset[3:0],[7:4]: 0x%x 0x%x\n", status.PgpTxPllRst[0], status.PgpTxPllRst[1]);


      printf(" PgpRxCommonPllReset[3:0],[7:4]: 0x%x 0x%x\n", status.PgpRxPllRst[0], status.PgpRxPllRst[1]);


      printf("PgpTxCommonPllLocked[3:0],[7:4]: 0x%x 0x%x\n", status.PgpTxPllRdy[0], status.PgpTxPllRdy[1]);


      printf("PgpRxCommonPllLocked[3:0],[7:4]: 0x%x 0x%x\n", status.PgpRxPllRdy[0], status.PgpRxPllRdy[1]);

      printf("     PgpLocLinkReady[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%u", status.PgpLocLinkReady[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("     PgpRemLinkReady[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u", status.PgpRemLinkReady[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      for(x=0;x<8;x++) {
        printf("       PgpRxCount[%u][0:3]:", x );
        for(y=0;y<4;y++) {
          printf("%u",status.PgpRxCount[x][y]);
          if(y!=3) printf(", "); else printf("\n");
        }
      }

      printf("       PgpCellErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpCellErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("      PgpLinkDownCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpLinkDownCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       PgpLinkErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpLinkErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       PgpFifoErrCnt[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%u",status.PgpFifoErrCnt[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("\n");
      printf("        EvrRunCode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunCode[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("     EvrAcceptCode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptCode[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("       EvrRunDelay[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunDelay[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("    EvrAcceptDelay[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptDelay[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("   EvrRunCodeCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrRunCodeCount[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      for(x=0;x<8;x++) {
        printf("EvrLutDropCount[%x][0:3]: ", x);
        for(y=0;y<4;y++) {
          printf("%d", ((status.EvrLutDropCount[x]>>(y*8))&0xff));
          if(y!=3) printf(", "); else printf("\n");
        }
      }

      printf("    EvrAcceptCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.EvrAcceptCount[x]);
        if (x==7) printf("\n"); else printf(", ");
      }


      printf("       EvrRunMask[0:7]:  ");
      for(x=0;x<8;x++){
        printf("%d", ((status.EvrRunMask>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("       EvrLaneMode[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneMode>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("  EvrLaneFiducials[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.EvrLaneFiducials[x]);
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("     EvrLaneEnable[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneEnable>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("     EvrLaneStatus[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", ((status.EvrLaneStatus>>x)&1));
        if (x==7) printf("\n"); else printf(", ");
      }

      printf("              EvrEnable: %d\n", status.EvrEnable);
      printf("               EvrReady: %d\n", status.EvrReady);
      printf("               EvrReset: %d\n", status.EvrReset);
      printf("              EvrPllRst: %d\n", status.EvrPllRst);
      printf("              EvrErrCnt: %d\n", status.EvrErrCnt);
      printf("              EvrFiducial: 0x%x\n", status.EvrFiducial);
      for(x=0;x<8;x++) {
        printf("  EvrEnHdrCheck[%d][0:3]: ", x);
        for(y=0;y<4;y++) {
          printf("%d", status.EvrEnHdrCheck[x][y]);
          if(y!=3) printf(", "); else printf("\n");
        }
      }
      printf("\n");

      printf("      TxDmaFull[0:7]: ");
      for(x=0;x<8;x++) {
        printf("%d", status.TxDmaAFull[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         TxReadReady: 0x%x\n", status.TxReadReady);
      printf("      TxRetFifoCount: 0x%x\n", status.TxRetFifoCount);
      printf("             TxCount: 0x%x\n", status.TxCount);
      printf("              TxRead: 0x%x\n", status.TxRead);
      printf("\n");

      printf("     RxFreeFull[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%d", status.RxFreeFull[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("    RxFreeValid[0:7]: ");
      for(x=0;x<8;x++) {
         printf("%d", status.RxFreeValid[x]);
        if(x!=7) printf(", "); else printf("\n");
      }

      printf("RxFreeFifoCount[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxFreeFifoCount[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         RxReadReady: 0x%x\n", status.RxReadReady);
      printf("      RxRetFifoCount: 0x%x\n", status.RxRetFifoCount);
      printf("             RxCount: 0x%x\n", status.RxCount);
      printf("        RxWrite[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxWrite[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("         RxRead[0:7]: ");
      for(x=0;x<8;x++) {
        printf("0x%x", status.RxRead[x]);
        if(x!=7) printf(", "); else printf("\n");
      }
      printf("\n");

    }
  }
}
