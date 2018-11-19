/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Configurator.cc
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

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/config/EventcodeTiming.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Configurator.hh"
#include "pds/epix10ka2m/Configurator.hh"
#include "pds/epix10ka2m/Destination.hh"
#include "pds/pgp/Reg.hh"
#include "pds/pgp/AxiVersion.hh"
#include <PgpDriver.h>
#include "ndarray/ndarray.h"

using Pds::Pgp::Reg;

namespace Pds {
  namespace Epix10ka2m {
    class CarrierId {
    public:
      Reg lo;
      Reg hi;
    };
    class SystemRegs {
    public:
      Reg usrRst;
      Reg dcDcEnable; // 4b
      Reg asicAnaEn;  // 1b
      Reg asicDigEn;  // 1b
      Reg ddrVttEn;
      Reg ddrVttPok;
      Reg tempAlert;
      Reg tempFault;
      Reg latchTempFault;
      uint32_t reserved_0x30[3];
      CarrierId carrierId[4];  // ASIC carriers
      uint32_t reserved_0x400[(0x400-0x50)>>2];
      Reg trigEn;
      Reg trigSrcSel; // 2b
      Reg autoTrigEn;
      Reg autoTrigPeriod;
      uint32_t reserved_0x500[(0x500-0x410)>>2];
      Reg adcClkRst;
      Reg adcReqStart;  // Reset and run the test
      Reg adcReqTest;   // Just run the test
      Reg adcTestDone;  // Test is done
      Reg adcTestFail;  // Test failed
      Reg adcChanFail[10]; // Which channels failed
    };
    class VguardDac {
    public:
      uint32_t reserved_0xc0[0xc0>>2];
      Reg dacRaw;  // raw = raw*((1<<16)-1)/(2.5*(1 + 845.0/3000))
    };
    class QuadMonitor {
    public:
      Reg monitorEn;
      Reg monitorStreamEn;
      Reg trigPrescaler;
      //  Remainder are readonly
      Reg shtError;
      Reg shtHumRaw;
      Reg shtTempRaw;
      Reg nctError;
      Reg nctLocTempRaw;
      Reg nctRemTempLRaw;
      Reg nctRemTempHRaw;
      uint32_t reserved_0x100[54];
      Reg ad7949DataRaw[8];
      uint32_t reserved_0x200[56];
      Reg sensorRegRaw[26];
    public:
    };
    class AcqCore {
    public:
      Reg acqCount;
      Reg acqCountReset;
      Reg acqToAsicR0Delay;
      Reg asicR0Width;
      Reg asicR0ToAsicAcq;
      Reg asicAcqWidth;
      Reg asicAcqLToPPmatL;
      Reg asicPPmatToReadout;
      Reg asicRoClkHalfT;
      Reg asicRoClkCount;
      Reg asicPreAcqTime;
      Reg asicForce;
      Reg asicValue;
    };
    class RdoutCore {
    public:
      Reg rdoutEn;
      Reg seqCount;
      Reg seqCountReset;
      Reg adcPipelineDelay;
      Reg lineBuffErr[4];
      Reg testData;
    };
    class PseudoScopeCore {
    public:
      Reg arm;
      Reg trig;
      Reg enable;
      Reg trigEdge;
      Reg trigChannel;
      Reg trigMode;
      Reg trigAdcThreshold;
      Reg trigHoldoff;
      Reg trigOffset;
      Reg traceLength;
      Reg skipSamples;
      Reg inChannelA;
      Reg inChannelB;
      Reg trigDelay;
    };
    class AdReadout {
    public:
      Reg channelDelay[8];
      Reg frameDelay;
      uint32_t reserved_0x30[3];
      Reg lockStatus;  // 0:15 lostLockCount, 16 locked
      Reg adcFrame;
      Reg lostLockCountReset;
      uint32_t reserved_0x80[(0x80-0x3c)>>2];
      Reg adcChannel[8];
      uint32_t reserved_0x00100000[(0x00100000-0xa0)>>2];
    };
    class AdConfig {
    public:
      uint32_t reserved_4;
      Reg chipId;
      Reg chipGrade;
      uint32_t reserved_0x10[1];
      Reg devIndexMask;
      Reg devIndexMaskDcoFco;
      uint32_t reserved_0x20[2];
      Reg pwdnMode;
      Reg dutyCycleStabilizer;
      uint32_t reserved_0x2c[1];
      Reg clockDivide;
      Reg chopMode;
      Reg testMode;
      uint32_t reserved_0x40[2];
      Reg offsetAdjust;
      uint32_t reserved_0x50[3];
      Reg outputInvFmt;
      uint32_t reserved_0x800[(0x800-0x54)>>2];
    public:
      void init();
      void setOutputFormat(unsigned f);
    };
    class AdcTester {
    public:
      Reg channel;
      Reg dataMask;
      Reg pattern;
      Reg samples;
      Reg timeout;
    };

    class Epix10kaAsic {
      enum { 
        _PrepRead    = 0x00000,
        _PixelData   = 0x05000,
        _RowCounter  = 0x06011,
        _ColCounter  = 0x06013,
        _PrepMulti   = 0x08000,
        _WriteMatrix = 0x84000,
      };
    public:
      Reg      reg[0x00100000];
    public:
      void clearMatrix() {
        reg[_PrepMulti  ] = 0;
        reg[_WriteMatrix] = 0;
        usleep(100000);
        reg[_PrepRead   ] = 0;
      }
      ndarray<uint16_t,2> getPixelMap() {
        unsigned shape[] = { 176,192 };
        ndarray<uint16_t,2> a(shape);
        reg[_PrepRead ]=0;
        reg[_PrepMulti]=0;
        for(unsigned x=0; x<shape[0]; x++)
          for(unsigned y=0; y<shape[1]; y++) {
            unsigned col  = y%48;
            if      (y< 48) col += 0x700;
            else if (y< 96) col += 0x680;
            else if (y<144) col += 0x580;
            else            col += 0x380;
            reg[_RowCounter] = x;
            reg[_ColCounter] = col;
            a[x][y] = reg[_PixelData];
          }
        return a;
      }
    };

    class Quad {
    public:
      Pds::Pgp::AxiVersion  _axiVersion;
      uint32_t              _reserved_0x00100000[(0x00100000-sizeof(Pds::Pgp::AxiVersion))>>2];
      SystemRegs            _systemRegs;
      uint32_t              _reserved_0x00500000[(0x00400000-sizeof(SystemRegs))>>2];
      VguardDac             _vguard_dac;
      uint32_t              _reserved_0x00700000[(0x00200000-sizeof(VguardDac))>>2];
      QuadMonitor           _quad_monitor;
      uint32_t              _reserved_0x01000000[(0x00900000-sizeof(QuadMonitor))>>2];
      AcqCore               _acqCore;
      uint32_t              _reserved_0x01100000[(0x00100000-sizeof(AcqCore))>>2];
      RdoutCore             _rdoutCore;
      uint32_t              _reserved_0x01200000[(0x00100000-sizeof(RdoutCore))>>2];
      PseudoScopeCore       _scopeCore;
      uint32_t              _reserved_0x02000000[(0x02000000-0x01200000-sizeof(PseudoScopeCore))>>2];
      AdReadout             _adReadout[10];
      AdConfig              _adConfigA[4];
      uint32_t              _reserved_0x02b00000[(0x02b00000-0x2a00000-4*sizeof(AdConfig))>>2];
      AdConfig              _adConfigB[4];
      uint32_t              _reserved_0x02c00000[(0x02c00000-0x2b00000-4*sizeof(AdConfig))>>2];
      AdConfig              _adConfigC[2];
      uint32_t              _reserved_0x02d00000[(0x02d00000-0x2c00000-2*sizeof(AdConfig))>>2];
      AdcTester             _adcTester;
      uint32_t              _reserved_0x04000000[(0x04000000-0x2d00000-sizeof(AdcTester))>>2];
      Epix10kaAsic          _asicSaci[16];
    public:
      static void dumpMap();
    public:
      AdConfig& adConfig(unsigned);
    };
  }
}

using namespace Pds::Epix10ka2m;

AdConfig& Quad::adConfig(unsigned i)
{
  AdConfig* p = 0;
  if      (i<4) p = &_adConfigA[i-0];
  else if (i<8) p = &_adConfigB[i-4];
  else          p = &_adConfigC[i-8];
  return *p;
}

void AdConfig::init()
{
  unsigned v = pwdnMode;
  v |= 0x3;
  pwdnMode = v;
  v &= ~0x3;
  pwdnMode = v;
}

void AdConfig::setOutputFormat(unsigned f)
{
  unsigned v = outputInvFmt;
  v &= ~1;
  v |= f&1;
  outputInvFmt = v;
}

void Quad::dumpMap()
{
  Quad* q = (Quad*)0;
  printf("Quad map\n");
  printf("\tAxiVersion  @ %p\n", &q->_axiVersion);
  printf("\tSystemRegs  @ %p\n", &q->_systemRegs);
  printf("\tVguardDac   @ %p\n", &q->_vguard_dac);
  printf("\tQuadMonitor @ %p\n", &q->_quad_monitor);
  printf("\tAcqCore     @ %p\n", &q->_acqCore);
  printf("\tRdoutCore   @ %p\n", &q->_rdoutCore);
  printf("\tPseudoScope @ %p\n", &q->_scopeCore);
  printf("\tAdReadout[0]@ %p\n", &q->_adReadout[0]);
  printf("\tAdReadout[1]@ %p\n", &q->_adReadout[1]);
  printf("\tAdConfigA[0]@ %p\n", &q->_adConfigA[0]);
  printf("\tAdConfigA[1]@ %p\n", &q->_adConfigA[1]);
  printf("\tAdConfigB[0]@ %p\n", &q->_adConfigB[0]);
  printf("\tAdcTester   @ %p\n", &q->_adcTester);
  printf("\tasicSaci[0] @ %p\n", &q->_asicSaci[0]);
  printf("\tasicSaci[1] @ %p\n", &q->_asicSaci[1]);
}


/*
//  configAddrs array elements are address:useModes

static uint32_t configAddrs[Epix10ka2MConfigShadow::NumberOfValues][2] = {
  {0x00,  1}, //  version
  {0x3d,  0}, //  usePgpEvr
  {0,		  2}, //  evrRunCode
  {0,     2}, //  evrDaqCode
  {0,     2}, //  evrRunTrigDelay
  {0x02,  0}, //  epixRunTrigDelay
  {0x07,  0}, //  DacSetting
  {0x29,  0}, //  AsicPins, etc
  {0x2a,  0}, //  AsicPinControl, etc
  {0x20,  0}, //  AcqToAsicR0Delay
  {0x21,  0}, //  AsicR0ToAsicAcq
  {0x22,  0}, //  AsicAcqWidth
  {0x23,  0}, //  AsicAcqLToPPmatL
  {0x3a,  0}, //  AsicPPmatToReadout
  {0x24,  0}, //  AsicRoClkHalfT
  {0x25,  0}, //  AdcReadsPerPixel
  {0x26,  0}, //  AdcClkHalfT
  {0x2b,  0}, //  AsicR0Width
  {0,     2}, //  AdcPipelineDelay
  {0x90,  0}, //  AdcPipelineDelay0
  {0x91,  0}, //  AdcPipelineDelay1
  {0x92,  0}, //  AdcPipelineDelay2
  {0x93,  0}, //  AdcPipelineDelay3
  {0,     2}, //  SyncParams
  {0x2e,  0}, //  PrepulseR0Width
  {0x2f,  0}, //  PrepulseR0Delay
  {0x30,  1}, //  DigitalCardId0
  {0x31,  1}, //  DigitalCardId1
  {0x32,  1}, //  AnalogCardId0
  {0x33,  1}, //  AnalogCardId1
  {0x3b,  1}, //  CarrierId0
  {0x3c,  1}, //  CarrierId1
  {0,     2}, //  NumberOfAsicsPerRow
  {0,     2}, //  NumberOfAsicsPerColumn
  {0,     2}, //  NumberOfRowsPerAsic
  {0,     2}, //  NumberOfReadableRowsPerAsic
  {0,     2}, //  NumberOfPixelsPerAsicRow
  {0,     2}, //  CalibrationRowCountPerASIC,
  {0,     2}, //  EnvironmentalRowCountPerASIC,
  {0x10,  1}, //  BaseClockFrequency
  {0xd,   0}, //  AsicMask
  {0x11,  0}, //  EnableAutomaticRunTrigger
  {0x12,  0}, //  NumberClockTicksPerRunTrigger
  {0x52,  0}, //  ScopeSetup1
  {0x53,  0}, //  ScopeSetup2
  {0x54,  0}, //  ScopeLengthAndSkip
  {0x55,  0}, //  ScopeInputSelects
};
*/

struct AsicRegMode {
  unsigned addr;
  unsigned mode;
};

typedef struct AsicRegMode AsicRegMode_s;

static AsicRegMode_s AconfigAddrs[Epix10kaASIC_ConfigShadow::NumberOfValues] = {
  {0x1001,  0},  //  pulserVsPixelOnDelay and pulserSync 0
  {0x1002,  0},  //  dummy pixel 1
  {0x1003,  0},  //  2
  {0x1004,  0},  //  3
  {0x1005,  0},  //  4
  {0x1006,  0},  //  5
  {0x1007,  0},  //  6
  {0x1008,  0},  //  7
  {0x1009,  0},  //  8
  {0x100a,  0},  //  9
  {0x100b,  0},  // 10
  {0x100c,  0},  // 11
  {0x100d,  0},  // 12
  {0x100e,  0},  // 13
  {0x100f,  0},  // 14
  {0x1010,  0},  // 15
  {0x1011,  3},  //  RowStartAddr 16
  {0x1012,  3},  //  RowStopAddr  17
  {0x1013,  3},  //  ColStartAddr 18
  {0x1014,  3},  //  ColStopAddr  19
  {0x1015,  1},  //  chipID       20
  {0x1016,  0},  //  21
  {0x1017,  0},  //  22
  {0x1018,  0},  //  23
  {0x1019,  0},  //  24
  {0x101A,  0}   //  25
};

static unsigned _writeElemAsicPCA(const Pds::Epix::Config10ka& e, 
                                  Epix10kaAsic*                saci);

static unsigned _checkElemAsicPCA(const Pds::Epix::Config10ka& e, 
                                  Epix10kaAsic*                saci,
                                  const char*                  base);

static unsigned _writeElemCalibPCA(const Pds::Epix::Config10ka& e, 
                                   Epix10kaAsic*                saci);


Configurator::Configurator(int f, unsigned lane, unsigned d) :
  Pds::Pgp::Configurator(true, f, d, lane),
  _q(0), _e(0), 
  _ewrote(new Pds::Epix::Config10ka[4]),
  _eread (new Pds::Epix::Config10ka[4]),
  _d(lane,Destination::Registers),
  _rhisto(0),
  _first(false)
{
  //  Initialize cache to impossible values
  memset(_ewrote,1,4*sizeof(Pds::Epix::Config10ka));
  memset(_eread ,1,4*sizeof(Pds::Epix::Config10ka));

  checkPciNegotiatedBandwidth();

#if 1
  Reg::setPgp(_pgp);
  Reg::setDest(_d.dest());

  Quad* q = 0;
  q->_quad_monitor.monitorEn       = 1;
  q->_quad_monitor.monitorStreamEn = 1;
  //  q->_quad_monitor.monitorEn       = 0;
  //  q->_quad_monitor.monitorStreamEn = 0;
#endif

  Quad::dumpMap();
}

Configurator::~Configurator() {
  evrLaneEnable(false);
}

unsigned Configurator::_resetFrontEnd() {
#if 0
  unsigned monitorEnable = 0;
  try {
    Quad* q = 0;
    monitorEnable = q->_quad_monitor.monitorEn & 0x1;
    q->_systemRegs.dcDcEnable = _q->dcdcEn   ();
    q->_systemRegs.asicAnaEn  = _q->asicAnaEn();
    q->_systemRegs.asicDigEn  = _q->asicDigEn();
    q->_systemRegs.ddrVttEn   = _q->ddrVttEn ();
    sleep(1);
    q->_quad_monitor.monitorEn       = monitorEnable;
    q->_quad_monitor.monitorStreamEn = monitorEnable;
  } 
  catch ( std::string& s) {
    printf("%s caught exception %s\n",
           __PRETTY_FUNCTION__, s.c_str());
  }
#endif
  // for(unsigned a=0; a<10; a++)
  //   q->_adConfig[a].init();
  // sleep(1);

#if 0
  // Reset deserializers
  q->_systemRegs.adcClkRst = 1;
  usleep(1);
  q->_systemRegs.adcClkRst = 0;
  sleep(1);
#endif
  return 0;
}

void Configurator::_resetSequenceCount() {
  if (_pgp) {
    Quad* pq = 0;
    _pgp->resetSequenceCount();
    pq->_acqCore.acqCountReset   = 1;
    pq->_rdoutCore.seqCountReset = 1;
    usleep(1);
    pq->_acqCore.acqCountReset   = 0;
    pq->_rdoutCore.seqCountReset = 0;
    usleep(1);
  } else {
    printf("Configurator::resetSequenceCount() found nil _pgp so not reset\n");
  }
}

uint32_t Configurator::_sequenceCount() {
  uint32_t count = -1U;
  Quad* q(0);
  if (_pgp) count = q->_rdoutCore.seqCount;
  else printf("Configurator::_sequenceCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Configurator::_acquisitionCount() {
  uint32_t count = -1U;
  Quad* q(0);
  if (_pgp) count = q->_acqCore.acqCount;
  else printf("Configurator::_acquisitionCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Configurator::enviroData(unsigned o) {
  Reg::setPgp(_pgp);
  Reg::setDest(_d.dest());
  uint32_t dat=1113;
#if 0
  if (_pgp) _pgp->readRegister(&_d, EnviroDataBaseAddr+o, 0x5e4, &dat);
  else printf("Configurator::enviroData() found nil _pgp so not read\n");
#endif
  return (dat);
}

void Configurator::_enableRunTrigger(bool f) {
  Quad* q = 0;
  q->_systemRegs.trigEn = f ? 1:0;
  _pgp->maskRunTrigger(!f);
  printf("Configurator::_enableRunTrigger(%s)\n", f ? "true" : "false");
}

void Configurator::print() {}

void Configurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool Configurator::_robustReadVersion(unsigned index) {
  return false;

  enum {numberOfTries=3};
  unsigned version = 0;
  unsigned failCount = 0;
  printf("\n\t--flush-%u-", index);
  while (failCount<numberOfTries) {
    try {
      Quad* q(0);
      version = q->_axiVersion._fwVersion;
      printf("%s version(0x%x)\n\t", _d.name(), version);
      return false;
    }
    catch( std::string& s ) {
      printf("%s(%u)-%s", _d.name(), ++failCount, s.c_str());
    }
  }

  if (index < 3) {
    _pgp->resetPgpLane();
    microSpin(10);
    return _robustReadVersion(++index);
  }

  printf("_robustReadVersion FAILED!!\n\t");
  return true;
}

unsigned Configurator::configure( const Epix::PgpEvrConfig&     p,
                                  const Epix10kaQuadConfig&     q,
                                  Epix::Config10ka*       a,
                                  unsigned first) 
{
  unsigned ret = 0;

  Reg::setPgp(_pgp);
  Reg::setDest(_d.dest());
#if 1
  printf("%s with PgpEvrConfig %p  QuadConfig %p  Elem %p\n",__PRETTY_FUNCTION__,&p,&q,a);

  _q = &q;
  _e = a;

  timespec      start, end;
  bool printFlag = true;
  clock_gettime(CLOCK_REALTIME, &start);
  //  _pgp->maskHWerror(true);
  printf("Configurator::configure %s reseting front end\n", first ? "" : "not ");
  if (first) {
    _resetFrontEnd();
    _first = true;
  }

  ret |= _robustReadVersion(0);

  //  Fetch some status for sanity
  { 
    Quad* pq = 0;
    printf("fwVersion: %x\n", unsigned(pq->_axiVersion._fwVersion));
    printf("buildSt  : %s\n", pq->_axiVersion.buildStamp().c_str());
    printf("upTime   : %x\n", unsigned(pq->_axiVersion._upTime));
    printf("ddrVttPok: %x\n", unsigned(pq->_systemRegs.ddrVttPok));
    printf("tempAlert: %x\n", unsigned(pq->_systemRegs.tempAlert));
    printf("tempFault: %x\n", unsigned(pq->_systemRegs.tempFault));

    for(unsigned i=0; i<4; i++)
      if (_e[i].asicMask()&0xf) {
        printf("carrierId[%u]: %08x:%08x\n", i, 
               unsigned(pq->_systemRegs.carrierId[i].hi), 
               unsigned(pq->_systemRegs.carrierId[i].lo));
      }
#if 0
    for(unsigned i=0; i<10; i++)
      printf("adConfig[%u]: %x:%x\n", i, 
             unsigned(pq->adConfig(i).chipId), 
             unsigned(pq->adConfig(i).chipGrade)&7);
#endif
    printf("SeqCount: %x\n", unsigned(pq->_rdoutCore.seqCount));
    printf("AcqCount: %x\n", unsigned(pq->_acqCore.acqCount));
    printf("AcqR0Wid: %x\n", unsigned(pq->_acqCore.asicR0Width));
    printf("AcqRoClk: %x\n", unsigned(pq->_acqCore.asicRoClkCount));
  }

  ret |= this->_G3config(p);

  //  ret |= _writeADCs();

  //  if (ret == 0) {
  if (1) {
    _resetSequenceCount();
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    _enableRunTrigger(false);
    if (printFlag) printf("\n\twriting top level config\n");
    ret |= _writeConfig();
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    if (printFlag) printf("\n\twriting ASIC regs\n");
    ret |= _writeASIC();
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    _enableRunTrigger(true);
    if (usleep(10000)<0) perror("Configurator::configure second ulseep failed\n");
  }
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone \n", ret);
    printf(" it took %lld.%lld milliseconds with first %u\n", diff/1000000LL, diff%1000000LL, first);
    if (ret) dumpFrontEnd();
  }
#else
  _enableRunTrigger(false);
  ret |= this->_G3config(p);
  _enableRunTrigger(true);
#endif

  return ret;
}

unsigned Configurator::_G3config(const Pds::Epix::PgpEvrConfig& c) {
  unsigned ret = 0;
  unsigned runTick = Pds_ConfigDb::EventcodeTiming::timeslot((unsigned)c.runCode());
  unsigned daqTick = Pds_ConfigDb::EventcodeTiming::timeslot((unsigned)c.daqCode());
  unsigned daqDelay = 0;
  if (runTick == daqTick) {
    daqDelay = (unsigned)c.runDelay() + 15;
  } else if (runTick > daqTick) {
    daqDelay = (runTick - daqTick) + (unsigned)c.runDelay() + 15;
  } else if (daqTick > runTick) {
    if ((daqTick - runTick) < ((unsigned)c.runDelay() + 64)) {
      daqDelay = ((unsigned)c.runDelay() + 64) + runTick - daqTick;
    } else {
      printf("%s Timing error!!!!!\n",__PRETTY_FUNCTION__);
      return 1;
    }
  }

  if (evrEnabled() == false) {
    evrEnable(true);
  }
  if (evrEnabled(true) == true) {
    ret |= evrEnableHdrChk(Destination::Data, true);
    ret |= evrRunCode((unsigned)c.runCode());
    ret |= evrRunDelay((unsigned)c.runDelay());
    ret |= evrDaqCode((unsigned)c.daqCode());
    ret |= evrDaqDelay(daqDelay);
    ret |=  waitForFiducialMode(false);
    microSpin(10);
    ret |= evrLaneEnable(false);
    microSpin(10);
    ret |=  waitForFiducialMode(true);
    printf("%s setting up fiber triggering on G3 pgpcard\n",__PRETTY_FUNCTION__);
    //    Quad* q = 0;
    //    q->_systemRegs.trigEn = 1;
  } else {
    ret = 1;
  }
  return ret;
}

unsigned Configurator::_writeConfig() 
{
  unsigned ret = Success;

  Quad* q = 0;
  q->_systemRegs.trigEn          = 0;
  q->_systemRegs.autoTrigEn      = 0;
  q->_systemRegs.trigSrcSel      = _q->trigSrcSel();
  q->_vguard_dac.dacRaw          = _q->vguardDac();
  q->_acqCore.acqToAsicR0Delay   = _q->acqToAsicR0Delay();
  q->_acqCore.asicR0Width        = _q->asicR0Width();
  q->_acqCore.asicR0ToAsicAcq    = _q->asicR0ToAsicAcq();
  q->_acqCore.asicAcqWidth       = _q->asicAcqWidth();
  q->_acqCore.asicAcqLToPPmatL   = _q->asicAcqLToPPmatL();
  q->_acqCore.asicPPmatToReadout = _q->asicPPmatToReadout();
  q->_acqCore.asicRoClkHalfT     = _q->asicRoClkHalfT();
  q->_acqCore.asicForce          = ((_q->asicAcqForce  ()<<0) |
                                     (_q->asicR0Force   ()<<1) |
                                     (_q->asicPPmatForce()<<2) |
                                     (_q->asicSyncForce ()<<3) |
                                     (_q->asicRoClkForce()<<4));
  q->_acqCore.asicValue          = ((_q->asicAcqValue  ()<<0) |
                                     (_q->asicR0Value   ()<<1) |
                                     (_q->asicPPmatValue()<<2) |
                                     (_q->asicSyncValue ()<<3) |
                                     (_q->asicRoClkValue()<<4));
  q->_rdoutCore.rdoutEn          = 1;
  q->_rdoutCore.adcPipelineDelay = _q->adcPipelineDelay();
  q->_rdoutCore.testData         = _q->testData();

  q->_scopeCore.enable           = _q->scopeEnable();
  // q->_scopeCore.arm              = 1;
  // q->_scopeCore.trig             = 1;
  q->_scopeCore.trigEdge         = _q->scopeTrigEdge();
  q->_scopeCore.trigChannel      = _q->scopeTrigChan();
  q->_scopeCore.trigMode         = _q->scopeTrigMode();
  q->_scopeCore.trigAdcThreshold = _q->scopeADCThreshold();
  q->_scopeCore.trigHoldoff      = _q->scopeTrigHoldoff();
  q->_scopeCore.trigOffset       = _q->scopeTrigOffset();
  q->_scopeCore.traceLength      = _q->scopeTraceLength();
  q->_scopeCore.skipSamples      = _q->scopeADCsamplesToSkip();
  q->_scopeCore.inChannelA       = _q->scopeChanAwaveformSelect();
  q->_scopeCore.inChannelB       = _q->scopeChanBwaveformSelect();
  q->_scopeCore.trigDelay        = _q->scopeTrigDelay();

  if (ret == Success)
    ret = _checkWrittenConfig(true);
  // if (ret == Success)
  //   q->_systemRegs.trigEn     = 1;

  return ret;
}

unsigned Configurator::_checkWrittenConfig(bool writeBack) {
  unsigned ret = Success;

  //  Readback configuration
  Quad* q = 0;
  {
    Epix10kaQuadConfig& c = *const_cast<Epix10kaQuadConfig*>(_q);
    uint32_t* u = reinterpret_cast<uint32_t*>(&c);
    u[3] = q->_axiVersion._deviceDna[0];
    u[4] = q->_axiVersion._deviceDna[1];
  }
  for(unsigned ia=0; ia<4; ia++)
    if (_e[ia].asicMask() & 0xf) {
      Epix::Config10ka& e = _e[ia];
      uint32_t* u = reinterpret_cast<uint32_t*>(&e);
      u[0] = q->_systemRegs.carrierId[ia].lo;
      u[1] = q->_systemRegs.carrierId[ia].hi;
    }


  if (!_pgp) printf("Configurator::_checkWrittenConfig found nil pgp\n");
  return ret;
}

unsigned Configurator::_writeADCs()
{
  Quad* q = 0;

  for(unsigned a=0; a<10; a++) {
    q->adConfig(a).devIndexMask       = _q->adc(a).devIndexMask();
    q->adConfig(a).devIndexMaskDcoFco = _q->adc(a).devIndexMaskDcoFco();
    q->adConfig(a).pwdnMode           = (((_q->adc(a).extPwdnMode()&1)<<5) |
                                          ((_q->adc(a).intPwdnMode ()&3)<<0));
    q->adConfig(a).dutyCycleStabilizer = _q->adc(a).dutyCycleStab();
    q->adConfig(a).clockDivide         = _q->adc(a).clockDivide();
    q->adConfig(a).chopMode            = (_q->adc(a).chopMode()&1)<<2;
    q->adConfig(a).testMode            = (((_q->adc(a).userTestMode()&0x3)<<6) |
                                           ((_q->adc(a).outputTestMode()&0xf)<<0));
    q->adConfig(a).offsetAdjust        = _q->adc(a).offsetAdjust();
    q->adConfig(a).outputInvFmt        = (((_q->adc(a).outputInvert()&1)<<2) |
                                           ((_q->adc(a).outputFormat()&1)<<0));
  }

  // apply ADC delays from configuration
  for(unsigned a=0; a<10; a++) {
    q->_adReadout[a].frameDelay = 0x200+_q->adc(a).frameDelay();
    ndarray<const uint32_t,1> channelDelay = _q->adc(a).channelDelay();
    for(unsigned c=0; c<8; c++)
      q->_adReadout[a].channelDelay[c] = 0x200+channelDelay[c];
    // enable output mode
    for(unsigned a=0; a<10; a++)
      q->adConfig(a).setOutputFormat(_q->adc(a).outputFormat());
  }

  // run the test
#if 0
  q->_systemRegs.adcReqStart = 1;
  usleep(1);
  q->_systemRegs.adcReqStart = 0;
  unsigned testDone=0,tmo=0;
  do {
    usleep(100);
    testDone = q->_systemRegs.adcTestDone & 1;
    if (++tmo > 100) {
      printf("Adc Test Timedout\n");
      return 1;
    }
  } while (testDone==0);
  printf("Adc Test Done after %u cycles\n",tmo);

  // check the results
  if (q->_systemRegs.adcTestFail != 0) {
    printf("%s: adc test failed\n",__PRETTY_FUNCTION__);
    unsigned v;
    for(unsigned a=0; a<10; a++)
      if ((v=q->_systemRegs.adcChanFail[a]))
        printf("\tChannel %d failed [%x]\n",a,v);
    //    return 1;
  }
#endif
  return 0;
}

unsigned Configurator::_writeASIC() 
{
  unsigned ret = Success;
  Quad* q = 0;
  for(unsigned ie=0; ie<4; ie++) {
    const Pds::Epix::Config10ka& e = _e[ie];
    unsigned m = e.asicMask();
    for (unsigned index=0; index<e.numberOfAsics(); index++) {
      if (m&(1<<index)) {
        uint32_t* u = (uint32_t*) &e.asics(index);
        unsigned ia = ie*4+index;
        try {
          for (unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfValues; i++) {
            unsigned addr = AconfigAddrs[i].addr;
            unsigned mode = AconfigAddrs[i].mode;
            Reg&     reg  = q->_asicSaci[ia].reg[addr];

            if (addr==0x1015) continue;  // skip chip ID

            if (mode == ReadOnly) {
              if (_debug & 1) printf("%s reading addr(%p)", __PRETTY_FUNCTION__, &reg);
              u[i] = reg;
              if (_debug & 1) printf(" data(0x%x) Asic(%u)\n", u[i], ia);
            }

            else if (mode==WriteOnly) {
              if (_debug & 1) printf("%s writing addr(%p) data(0x%x)", __PRETTY_FUNCTION__, &reg, u[i]);
              reg = u[i];
            }

            else {
              if (_debug & 1) printf("%s writing addr(%p) data(0x%x)", __PRETTY_FUNCTION__, &reg, u[i]);
              reg = u[i];
              unsigned v = reg;
              if (_debug & 1) printf("%s read addr(%p) data(0x%x)", __PRETTY_FUNCTION__, &reg, v);
            }
          }
        }
        catch(std::string& e) {
          printf("Caught exception: %s\n",e.c_str());
          ret |= Failure;
        }
      }
    }
  }

  ret |= _writePixelBits();

  if (ret==Success) 
    ret |= _checkWrittenASIC(true);

  return ret;
}

static const unsigned WriteAhead = 1;

#define CHKWRITE                                                           \
  if (++writeCount >= WriteAhead) {                                       \
    __attribute__((unused)) volatile unsigned v = asic.reg[PixelDataAddr]; \
    writeCount = 0;                                                     \
  }

unsigned Configurator::_writePixelBits() 
{
  unsigned ret = Success;
  timespec tv; clock_gettime(CLOCK_REALTIME,&tv);
  bool first = _first;
  _first = false;

  Quad* q = 0;

  for(unsigned ie=0; ie<4; ie++) {
    const Pds::Epix::Config10ka& e = _e[ie];

    //  Skip configuration if all ASICs masked off
    unsigned m    = e.asicMask();
    if (!m) continue;

    //  Skip asic pixel configuration if same as before
    if (memcmp(_ewrote[ie].asicPixelConfigArray().data(),
               e.asicPixelConfigArray().data(),
               e.asicPixelConfigArray().size()*sizeof(uint16_t))!=0) {
      memcpy(const_cast<uint16_t*>(_ewrote[ie].asicPixelConfigArray().data()),
             e.asicPixelConfigArray().data(),
             e.asicPixelConfigArray().size()*sizeof(uint16_t));

      ret |= _writeElemAsicPCA(e, &q->_asicSaci[4*ie]);

      if (_debug & (1<<4)) {  // check written pixel bits (slow!)
        char buff[64];
        sprintf(buff,"/tmp/epix.%08x.l%x.e%x",
                unsigned(tv.tv_sec), _pgp->portOffset()+_d.lane(), ie);

        ret |= _checkElemAsicPCA(e, &q->_asicSaci[4*ie], buff);
      }
    }

    //  Skip calib pixel configuration if same as before
    if (memcmp(_ewrote[ie].calibPixelConfigArray().data(),
               e.calibPixelConfigArray().data(),
               e.calibPixelConfigArray().size()*sizeof(uint16_t))!=0) {
      memcpy(const_cast<uint8_t*>(_ewrote[ie].calibPixelConfigArray().data()),
             e.calibPixelConfigArray().data(),
             e.calibPixelConfigArray().size()*sizeof(uint8_t));

      ret |= _writeElemCalibPCA(e, &q->_asicSaci[4*ie]);
    }

    try {
      printf("Configurator::writePixelBits PrepareForReadout for ASIC:");
      for (unsigned index=0; index<e.numberOfAsics(); index++) {
        if (m&(1<<index)) {
          q->_asicSaci[4*ie+index].reg[PrepareForReadout] = 0;
          __attribute__((unused)) volatile unsigned v = q->_asicSaci[4*ie+index].reg[PrepareForReadout];
        }
      }
    }
    catch(std::string& exc) {
      printf("%s\n",exc.c_str());
      printf("Configurator::writePixelBits failed on PrepareForReadout\n");
      ret |= Failure;
    }
  }
  return ret;
}

unsigned Configurator::_checkWrittenASIC(bool writeBack) {
  unsigned ret = Success;
  Quad* q = 0;
  for(unsigned ie=0; ie<4; ie++) {
    const Pds::Epix::Config10ka& e = _e[ie];
    bool done = false;
    while (!done && _e && _pgp) {
      printf("Configurator::_checkWrittenASIC\n");
      unsigned m = e.asicMask();
      uint32_t myBuffer[sizeof(Epix10kaASIC_ConfigShadow)/sizeof(uint32_t)];
      Epix10kaASIC_ConfigShadow* readAsic = (Epix10kaASIC_ConfigShadow*) myBuffer;
      for (unsigned index=0; index<e.numberOfAsics(); index++) {
        if (m&(1<<index)) {
          Epix10kaAsic& asic = q->_asicSaci[4*ie+index];
          for (int i = 0; (i<Epix10kaASIC_ConfigShadow::NumberOfValues) && (ret==Success); i++)
            if ((AconfigAddrs[i].mode != WriteOnly) && _pgp)
              myBuffer[i] = unsigned(asic.reg[AconfigAddrs[i].addr])&0xffff;
          printf("checking elem(%u) asic(%u)\n",ie,index);
          Epix10kaASIC_ConfigShadow* confAsic = (Epix10kaASIC_ConfigShadow*) &(e.asics(index));
          if ( (ret==Success) && (*confAsic != *readAsic) && _pgp) {
            printf("Configurator::_checkWrittenASIC failed on ASIC %u\n", index);
            if (writeBack) *confAsic = *readAsic;
          }
        }
      }
      done = true;
    }
  }
  if (!_pgp) printf("Configurator::_checkWrittenASIC found nil pgp\n");
  return ret;
}

// 0 Thermistor 0 temperature in Celcius degree * 100. Signed data.
// 1 Thermistor 1 temperature in Celcius degree * 100. Signed data.
// 2 Relative humidity in percent * 100. Signed data.
// 3 ASIC analog current in mA. Unsigned data.
// 4 ASIC digital current in mA. Unsigned data.
// 5 Detector guard ring current in uA. Unsigned data.
// 6 Detector bias current in uA. Unsigned data.
// 7 Analog input voltage in mV. Unsigned data.
// 8 Digital input voltage in mV. Unsigned data.
#define NumberOfEnviroDatas 9

static char enviroNames[NumberOfEnviroDatas+1][120] = {
  {"Thermistor 0 in degrees Celcius...."},
  {"Thermistor 1 in degrees Celcius...."},
  {"Relative humidity in percent......."},
  {"ASIC analog current in mA.........."},
  {"ASIC digital current in mA........."},
  {"Detector guard ring current in uA.."},
  {"Detector bias current in uA........"},
  {"Analog input voltage in mV........."},
  {"Digital input voltage in mV........"}
};

void Configurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    pgp()->printStatus();
    printf("\tSequence Count(%u), Acquisition Count(%u)\n", _sequenceCount(), _acquisitionCount());
    printf("Environmental Data:\n");
    for (int i=0; i<NumberOfEnviroDatas; i++) {
      if (i<3) printf("\t%s%5.2f\n", enviroNames[i], 0.01*(signed int)enviroData(i));
      else printf("\t%s%5d\n", enviroNames[i], enviroData(i));
    }
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("Configurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}

unsigned Configurator::unconfigure()
{
  return 0;
}

unsigned _writeElemAsicPCA(const Pds::Epix::Config10ka& e, 
                           Epix10kaAsic*                saci)
{
  unsigned ret = 0;

  unsigned writeCount = 0;
  const unsigned rows = e.numberOfRows();
  const unsigned cols = e.numberOfColumns();
  unsigned offset = 0;
  unsigned bank = 0;
  unsigned bankOffset = 0;
  const unsigned banks[4] = {Bank0, Bank1, Bank2, Bank3};
  unsigned myRow = 0;
  unsigned myCol = 0;
  unsigned bankHisto[4] = {0,0,0,0};
  unsigned asicHisto[4] = {0,0,0,0};
  uint32_t thisPix = 0;
  
  unsigned pops[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned totPixels = 0;
  // find the most popular pixel setting
  for (unsigned r=0; r<rows; r++) {
    for (unsigned c=0; c<cols; c++) {
      pops[e.asicPixelConfigArray()(r,c)&15] += 1;
      totPixels += 1;
    }
  }
  unsigned max = 0;
  uint32_t pixel = e.asicPixelConfigArray()(0,0) & 0xffff;
  printf("\nConfigurator::writePixelBits() histo");
  for (unsigned n=0; n<16; n++) {
    printf(" %u,", pops[n]);
    if (pops[n] > max) {
      max = pops[n];
      pixel = n;
    }
  }
  printf(" pixel %u, total pixels %u\n", pixel, totPixels);
  // if (first) {
  //   if ((totPixels != pops[pixel]) || (pixel)) {
  //     usleep(1400000);
  //     printf("Configurator::writePixelBits sleeping 1.4 seconds\n");
  //   }
  // }

  // program the entire array to the most popular setting
  // note, that this is necessary because the global pixel write does not work in this device and programming
  //   one pixel in one bank also programs the same pixel in the other banks
  for (unsigned index=0; index<e.numberOfAsics(); index++)
    if (e.asicMask()&(1<<index)) {
      Epix10kaAsic& asic = saci[index];
      asic.reg[PrepareMultiConfigAddr] = 0;
      CHKWRITE;
    }
  for (unsigned index=0; index<e.numberOfAsics(); index++)
    if (e.asicMask()&(1<<index)) {
      Epix10kaAsic& asic = saci[index];
      asic.reg[WriteWholeMatricAddr] = pixel;
      CHKWRITE; 
    }

  // allow it queue up writeAhead commands
  unsigned row=0,col=0;
  try {
    for (row=0; row<rows; row++) {
      for (col=0; col<cols; col++) {
        if ((thisPix = (e.asicPixelConfigArray()(row,col)&0xf)) != pixel) {
          if (row >= (rows>>1)) {
            if (col < (cols>>1)) {
              offset = 3;
              myCol = col;
              myRow = row - (rows>>1);
            }
            else {
              offset = 0;
              myCol = col - (cols>>1);
              myRow = row - (rows>>1);
            }
          } else {
            if (col < (cols>>1)) {
              offset = 2;
              myCol = ((cols>>1)-1) - col;
              myRow = ((rows>>1)-1) - row;
            }
            else {
              offset = 1;
              myCol = (cols-1) - col;
              myRow = ((rows>>1)-1) - row;
            }
          }
          if (e.asicMask()&(1<<offset)) {
            bank = (myCol % (PixelsPerBank<<2)) / PixelsPerBank;
            bankOffset = banks[bank];
            bankHisto[bank]+= 1;
            asicHisto[offset] += 1;

            Epix10kaAsic& asic = saci[offset];
            asic.reg[RowCounterAddr] = myRow;                                CHKWRITE;
            asic.reg[ColCounterAddr] = bankOffset | (myCol % PixelsPerBank); CHKWRITE;
            asic.reg[PixelDataAddr ] = thisPix;                              CHKWRITE;
          }
        }
      }
    }
  }
  catch(std::string& exc) {
    printf("%s\n",exc.c_str());
    printf("Configurator::writePixelBits failed on row %u, col %u\n", row, col);
    ret |= Configurator::Failure;
  }
  printf("Configurator::writePixelBits banks %u %u %u %u\n",
         bankHisto[0], bankHisto[1],bankHisto[2],bankHisto[3]);
  printf("Configurator::writePixelBits asics %u %u %u %u\n",
         asicHisto[0], asicHisto[1],asicHisto[2],asicHisto[3]);
  return ret;
}

unsigned _checkElemAsicPCA(const Pds::Epix::Config10ka& e, 
                           Epix10kaAsic*                saci,
                           const char*                  base)
{
  ndarray<const uint16_t,2> pca = e.asicPixelConfigArray();
  for (unsigned index=0; index<e.numberOfAsics(); index++)
    if (e.asicMask()&(1<<index)) {
      ndarray<uint16_t,2> a = saci[index].getPixelMap();
      unsigned nprint=4, nerr=0;
      FILE *fwr, *frd;
      { char buff[64];
        sprintf(buff,"%s.a%x.wr", base, index);
        fwr = fopen(buff,"w");
        sprintf(buff,"%s.a%x.rd", base, index);
        frd = fopen(buff,"w"); }
      for(unsigned r=0; r<a.shape()[0]; r++) {
        for(unsigned c=0; c<a.shape()[1]; c++) {
          unsigned ar=0,ac=0;
          switch(index) {
          case 0: ar = a.shape()[0] +r   ; ac = a.shape()[1]   +c   ; break;
          case 1: ar = a.shape()[0] -r -1; ac = a.shape()[1]*2 -c -1; break;
          case 2: ar = a.shape()[0] -r -1; ac = a.shape()[1]   -c -1; break;
          case 3: ar = a.shape()[0] +r   ; ac = c                   ; break;
          }
          if (a[r][c] != pca[ar][ac]) {
            nerr++;
            if (nprint) {
              printf("  PCA error [%u][%u]  wrote(0x%x) read(0x%x)\n", ar, ac, pca[ar][ac], a[r][c]);
              nprint--;
            }
          }
          fprintf(fwr,"%x",pca[ar][ac]);
          fprintf(frd,"%x",a[r][c]);
        }
        fprintf(fwr,"\n");
        fprintf(frd,"\n");
      }
      fclose(fwr);
      fclose(frd);
      printf(" %u PCA errors\n",nerr);
    }
  return Configurator::Success;
}

unsigned _writeElemCalibPCA(const Pds::Epix::Config10ka& e, 
                            Epix10kaAsic*                saci)
{
  unsigned ret = 0;

  unsigned writeCount = 0;
  const unsigned cols = e.numberOfColumns();
  unsigned row=0,col=0;
  unsigned offset = 0;
  unsigned bank = 0;
  unsigned bankOffset = 0;
  const unsigned banks[4] = {Bank0, Bank1, Bank2, Bank3};
  unsigned myCol = 0;
  
  //  there is one pixel configuration row per each pair of calibration rows.
  unsigned calibRow = 176;
  try {
    for (row=0; row<2; row++) {
      printf("Configurator::writePixelBits calibration row 0x%x\n", row);
      for (col=0; col<cols; col++) {
        unsigned pixel = e.calibPixelConfigArray()(row,col) & 3;
        if (row) {
          if (col < (cols>>1)) {
            offset = 3;
            myCol = col;
          }
          else {
            offset = 0;
            myCol = col - (cols>>1);
          }
        } else {
          if (col < (cols>>1)) {
            offset = 2;
            myCol = ((cols>>1)-1) - col;
          }
          else {
            offset = 1;
            myCol = (cols-1) - col;
          }
        }
        if (e.asicMask()&(1<<offset)) {
          bank = (myCol % (PixelsPerBank<<2)) / PixelsPerBank;
          bankOffset = banks[bank];

          Epix10kaAsic& asic = saci[offset];
          asic.reg[RowCounterAddr] = calibRow;                             CHKWRITE;
          asic.reg[ColCounterAddr] = bankOffset | (myCol % PixelsPerBank); CHKWRITE;
          asic.reg[PixelDataAddr ] = pixel;                                CHKWRITE;
        }
      }
    }
  }
  catch(std::string& exc) {
    printf("%s\n",exc.c_str());
    printf("Configurator::writePixelBits failed on row %u, col %u\n", row, col);
    ret |= Configurator::Failure;
  }
  return ret;
}

