/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Configurator.hh
-- Author     : Matt Weaver <weaver@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2018-09-28
-- Last update: 2018-09-17
-- Platform   : 
-- Standard   : C++
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- This file is part of 'LCLS DAQ'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'LCLS DAQ', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------*/

#ifndef Pds_Epix10ka2m_Configurator_hh
#define Pds_Epix10ka2m_Configurator_hh

#include "pds/pgp/Configurator.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/Reg.hh"
#include "pds/epix10ka/Epix10kaStatusRegisters.hh"
#include "pds/epix10ka2m/Destination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds {
  namespace Epix10ka2m {
    class Epix10kaAsic;
    //    AcqCount : 0x000005
    //    Can be reset by writing to 0x000006
    //
    //    SeqCount : 0x00000B
    //    Can be reset by writing to 0x00000C
    //    0x000011, bit 0 - enable automatic run triggers (only works if the normal RunTrigEnable at register 0x000001 is also set)
    //    0x000012, bits 31:0 - number of clock cycles between autotriggers (for 120 Hz, this should be set to 1041667, or 0xFE503)
    //    0x000013, bit 0 - enable automatic daq triggers (only works if the normal DaqTrigEnable at register 0x000003 is also set)

    //    There is a PGP status register block that starts at the offset 0x00300000. The register map is as follows (check bits Remote Pause Status in 0x20):
    //
    //    -- Address map (offset from base):
    //    --    0x00 = Read/Write
    //    --       Bits 0 = Count Reset
    //    --    0x04 = Read/Write
    //    --       Bits 0 = Reset Rx
    //    --    0x08 = Read/Write
    //    --       Bits 0 = Flush
    //    --    0x0C = Read/Write
    //    --       Bits 1:0 = Loop Back
    //    --    0x10 = Read/Write
    //    --       Bits 7:0 = Sideband data to transmit
    //    --       Bits 8   = Sideband data enable
    //    --    0x14 = Read/Write
    //    --       Bits 0 = Auto Status Send Enable (PPI)
    //    --    0x18 = Read/Write
    //    --       Bits 0 = Disable Flow Control
    //    --    0x20 = Read Only
    //    --       Bits 0     = Rx Phy Ready
    //    --       Bits 1     = Tx Phy Ready
    //    --       Bits 2     = Local Link Ready
    //    --       Bits 3     = Remote Link Ready
    //    --       Bits 4     = Transmit Ready
    //    --       Bits 9:8   = Receive Link Polarity
    //    --       Bits 15:12 = Remote Pause Status
    //    --       Bits 19:16 = Local Pause Status
    //    --       Bits 23:20 = Remote Overflow Status
    //    --       Bits 27:24 = Local Overflow Status
    //    --    0x24 = Read Only
    //    --       Bits 7:0 = Remote Link Data
    //    --    0x28 = Read Only
    //    --       Bits ?:0 = Cell Error Count
    //    --    0x2C = Read Only
    //    --       Bits ?:0 = Link Down Count
    //    --    0x30 = Read Only
    //    --       Bits ?:0 = Link Error Count
    //    --    0x34 = Read Only
    //    --       Bits ?:0 = Remote Overflow VC 0 Count
    //    --    0x38 = Read Only
    //    --       Bits ?:0 = Remote Overflow VC 1 Count
    //    --    0x3C = Read Only
    //    --       Bits ?:0 = Remote Overflow VC 2 Count
    //    --    0x40 = Read Only
    //    --       Bits ?:0 = Remote Overflow VC 3 Count
    //    --    0x44 = Read Only
    //    --       Bits ?:0 = Receive Frame Error Count
    //    --    0x48 = Read Only
    //    --       Bits ?:0 = Receive Frame Count
    //    --    0x4C = Read Only
    //    --       Bits ?:0 = Local Overflow VC 0 Count
    //    --    0x50 = Read Only
    //    --       Bits ?:0 = Local Overflow VC 1 Count
    //    --    0x54 = Read Only
    //    --       Bits ?:0 = Local Overflow VC 2 Count
    //    --    0x58 = Read Only
    //    --       Bits ?:0 = Local Overflow VC 3 Count
    //    --    0x5C = Read Only
    //    --       Bits ?:0 = Transmit Frame Error Count
    //    --    0x60 = Read Only
    //    --       Bits ?:0 = Transmit Frame Count
    //    --    0x64 = Read Only
    //    --       Bits 31:0 = Receive Clock Frequency
    //    --    0x68 = Read Only
    //    --       Bits 31:0 = Transmit Clock Frequency
    //    --    0x70 = Read Only
    //    --       Bits 7:0 = Last OpCode Transmitted
    //    --    0x74 = Read Only
    //    --       Bits 7:0 = Last OpCode Received
    //    --    0x78 = Read Only
    //    --       Bits ?:0 = OpCode Transmit count
    //    --    0x7C = Read Only
    //    --       Bits ?:0 = OpCode Received count




    enum contolValues {
      disable = 0,
      enable  = 1
    };


    enum controlAddrs {
      ResetAddr                         = 0x0,
      DaqTriggerEnable                  = 0x3,
      RunTriggerEnable                  = 0x1,
      ResetFrameCounter                 = 0xC,
      ReadFrameCounter                  = 0xB,
      ResetAcqCounter                   = 0x6,
      ReadAcqCounter                    = 0x5,
      PowerEnableAddr                   = 0x8,
      PowerEnableValue                  = 0x7,
      AdcControlAddr                    = 0x80,
      AdcCtrlReqMask                    = 1,
      AdcCtrlAckMask                    = 2,
      AdcCtrlFailMask                   = 4,
      EnviroDataBaseAddr				        = 0x140,
      RowCounterAddr                    = 0x6011,
      ColCounterAddr                    = 0x6013,
      PixelDataAddr                     = 0x5000,
      WriteColDataAddr                  = 0x3000,
      PrepareMultiConfigAddr            = 0x8000,
      WriteWholeMatricAddr              = 0x84000,
      MultiplePixelWriteCommandAddr     = 0x080000,
      TotalPixelsAddr                   = 0x27,
      PixelsPerBank                     = 48,
      BanksPerAsic                      = 4,
      Bank0                             = ((unsigned)(0xe<<7)),
      Bank1                             = ((unsigned)(0xd<<7)),
      Bank2                             = ((unsigned)(0xb<<7)),
      Bank3                             = ((unsigned)(0x7<<7)),
      SaciClkBitAddr                    = 0x28,
      SaciClkBitValue                   = 0x4,
      TopCalibRow                       = 0x10000, //0x00010000
      BottomCalibRow                    = 0x30000,
      //      NumberClockTicksPerRunTrigger     = 0xFE503, // 120 Hz  add to config
      //                                        // 0x1fca05 // 60 Hz
      //                                        // 0x3f940b // 30 Hz
      //                                        // 0xbebc20 // 10 Hz
      //                                        //0x17d7840 //  5 Hz
      //                                        //0x7735940 //  1 Hz
      EnableAutomaticDaqTriggerAddr     = 0x13,  // do not add to config
      DaqTrigggerDelayAddr              = 0x4,
      RunToDaqTriggerDelay              = 15,
      PgpTriggerEnable                  = 0x3d,
      MonitorEnableAddr                 = 0x3e
    };

    enum asicControlAddrs {
      AsicQuadAddrBase     = 0x04000000,
      //      AsicAddrBase         = 0x800000,
      AsicAddrOffset       = 0x00100000,
      WritePixelCommand     =  0x8000,
      GlobalPixelCommand    =  0x4000,
      ConfigureAPixelCommand = 0x6011,
      PixelColumnCommand     = 0x6013,
      PrepareForReadout =        0,
      RepeatControlCount = 1
    };

    enum useModes { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };

    enum enables { Disable=0, Enable=1 };

    class Quad;

    class Configurator : public Pds::Pgp::Configurator {
    public:
      Configurator(int fd, unsigned lane, unsigned quad, unsigned debug);
      virtual ~Configurator();

      enum resultReturn {Success=0, Failure=1, Terminate=2};

      unsigned             configure(const Epix::PgpEvrConfig&,
                                     const Epix10kaQuadConfig&,
                                     Epix::Config10ka*,
                                     unsigned first=0);
      unsigned             unconfigure();
      //      Epix::Config10ka*    configuration() const;
      void                 print();
      void                 dumpFrontEnd();
      void                 printMe();
      uint32_t             enviroData(unsigned);
    private:
      unsigned             _resetFrontEnd();
      unsigned             _enableMonitoring(bool);
      void                 _resetSequenceCount();
      uint32_t             _sequenceCount();
      uint32_t             _acquisitionCount();
      void                 _enableRunTrigger(bool);
      unsigned             _writeConfig();
      unsigned             _checkWrittenConfig(bool writeBack=true);
      unsigned             _writeADCs();
      unsigned             _checkADCs();
      unsigned             _writeASIC();
      unsigned             _writePixelBits();
      unsigned             _checkWrittenASIC(bool writeBack=true);
      unsigned             _checkIsEnASIC();
      unsigned             _G3config(const Pds::Epix::PgpEvrConfig&);
      unsigned             _robustReadVersion(unsigned index=0);
      unsigned             _writeElemAsicPCA(const Pds::Epix::Config10ka& e, 
                                             Epix10kaAsic*                saci);
      unsigned             _checkElemAsicPCA(const Pds::Epix::Config10ka& e, 
                                             Epix10kaAsic*                saci,
                                             const char*                  base);
      unsigned             _writeElemCalibPCA(const Pds::Epix::Config10ka& e, 
                                              Epix10kaAsic*                saci);

    private:
      enum {MicroSecondsSleepTime=50};
      Pds::Pgp::SrpV3::Protocol*    _protocol;
      const Epix10kaQuadConfig*     _q;
      Pds::Epix::Config10ka*        _e;
      Pds::Epix::Config10ka*        _ewrote;
      Pds::Epix::Config10ka*        _eread;
      Destination                   _d;
      unsigned                      _quad;
      unsigned*                     _rhisto;
      bool                          _first;
    };
  }
}

#endif
