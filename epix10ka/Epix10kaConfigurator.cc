/*
 * Epix10kaConfigurator.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

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
#include "pds/epix10ka/Epix10kaConfigurator.hh"
#include "pds/epix10ka/Epix10kaDestination.hh"
#include "pds/epix10ka/Epix10kaStatusRegisters.hh"
#include "ndarray/ndarray.h"

using namespace Pds::Epix10ka;

class Epix10kaDestination;

//  configAddrs array elements are address:useModes

static uint32_t configAddrs[Epix10kaConfigShadow::NumberOfValues][2] = {
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

static uint32_t AconfigAddrs[Epix10kaASIC_ConfigShadow::NumberOfValues][2] = {
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


Epix10kaConfigurator::Epix10kaConfigurator(int f, unsigned d) :
                               Pds::Pgp::Configurator(true, f, d),
                               _testModeState(0), _config(0), _s(0), _rhisto(0),
                               _maintainLostRunTrigger(0), _fiberTriggering(false), _first(false) {
  allocateVC(7);
  checkPciNegotiatedBandwidth();
  _d.dest(Epix10kaDestination::Registers);  // Turn on monitoring immediately.
  _pgp->writeRegister(&_d, 1, 1);
  _pgp->writeRegister(&_d, MonitorEnableAddr, 1);
  printf("Epix10kaConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

Epix10kaConfigurator::~Epix10kaConfigurator() {
  evrLaneEnable(false);
}

unsigned Epix10kaConfigurator::resetFrontEnd() {
  unsigned count = 0;
  unsigned monitorEnable = 0;
  _d.dest(Epix10kaDestination::Registers);
  _pgp->readRegister(&_d, MonitorEnableAddr, 0xf00, &monitorEnable, 1);
  monitorEnable &= 1;
  _pgp->writeRegister(&_d, ResetAddr, 1);
  usleep(100000);
  uint32_t returned = 0;
  do {
    usleep(10000);
    _pgp->readRegister(&_d, AdcControlAddr, 0x5e1, &returned);
  } while (((returned & AdcCtrlAckMask) == 0) && (count++ < 10));
  if ((returned & AdcCtrlFailMask) != 0) {
    printf("Epix10kaConfigurator::resetFrontEnd found that ADC alignment FAILED!!! returned %d\n", returned);
    return resyncADC(1);
  }
  _pgp->writeRegister(&_d, MonitorEnableAddr, monitorEnable);
  printf("Epix10kaConfigurator::resetFrontEnd found ADC alignment Succeeded, returned %d\n", returned);
  return 0;
}

unsigned Epix10kaConfigurator::resyncADC(unsigned c) {
  unsigned ret = 0;
  unsigned count = 0;
  _d.dest(Epix10kaDestination::Registers);
  _pgp->writeRegister(&_d, AdcControlAddr, 0);
  microSpin(10);
  _pgp->writeRegister(&_d, AdcControlAddr, AdcCtrlReqMask);
  microSpin(10);
  //	_pgp->writeRegister(&_d, AdcControlAddr, 0);
  uint32_t returned = 0;
  do {
    usleep(100000);
    _pgp->readRegister(&_d, AdcControlAddr, 0x5e1, &returned);
  } while (((returned & AdcCtrlAckMask) == 0) && (count++ < 10));
  if ((returned & AdcCtrlFailMask) != 0) {
    printf("\tEpix10kaConfigurator::resyncADC found that ADC alignment FAILED!!! returned %d\n", returned);
    if (c>0) ret = resyncADC(c-1);
    else ret = 1;
  } else {
    printf("\tEpix10kaConfigurator::resyncADC found ADC alignment Succeeded, returned %d\n", returned);
  }
  return ret;
}

void Epix10kaConfigurator::resetSequenceCount() {
  _d.dest(Epix10kaDestination::Registers);
  if (_pgp) {
    if(fiberTriggering()) {
      _pgp->resetSequenceCount();
    }
    _pgp->writeRegister(&_d, ResetFrameCounter, 1);
    _pgp->writeRegister(&_d, ResetAcqCounter, 1);
    microSpin(10);
  } else {
    printf("Epix10kaConfigurator::resetSequenceCount() found nil _pgp so not reset\n");
  }
}

uint32_t Epix10kaConfigurator::sequenceCount() {
  _d.dest(Epix10kaDestination::Registers);
  uint32_t count=1111;
  if (_pgp) _pgp->readRegister(&_d, ReadFrameCounter, 0x5e4, &count);
  else printf("Epix10kaConfigurator::sequenceCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Epix10kaConfigurator::acquisitionCount() {
  _d.dest(Epix10kaDestination::Registers);
  uint32_t count=1112;
  if (_pgp) _pgp->readRegister(&_d, ReadAcqCounter, 0x5e5, &count);
  else printf("Epix10kaConfigurator::acquisitionCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Epix10kaConfigurator::enviroData(unsigned o) {
  _d.dest(Epix10kaDestination::Registers);
  uint32_t dat=1113;
  if (_pgp) _pgp->readRegister(&_d, EnviroDataBaseAddr+o, 0x5e4, &dat);
  else printf("Epix10kaConfigurator::enviroData() found nil _pgp so not read\n");
  return (dat);
}

void Epix10kaConfigurator::enableExternalTrigger(bool f) {
  _d.dest(Epix10kaDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, DaqTriggerEnable, f ? Enable : Disable);
  } else printf("Epix10kaConfigurator::enableExternalTrigger(%s) found nil _pgp so not set\n", f ? "true" : "false");
}

void Epix10kaConfigurator::enableRunTrigger(bool f) {
  unsigned mask = 1<<(_pgp->portOffset());
  _d.dest(Epix10kaDestination::Registers);
  _pgp->writeRegister(&_d, RunTriggerEnable, f ? Enable : Disable);
  _pgp->maskRunTrigger(f ? false : true);
  printf("Epix10kaConfigurator::enableRunTrigger(%s), mask 0x%x\n", f ? "true" : "false", mask);
}

void Epix10kaConfigurator::print() {}

void Epix10kaConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

void Epix10kaConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("Epix10kaConfigurator::runTimeConfigName(%s)\n", name);
}

bool Epix10kaConfigurator::_robustReadVersion(unsigned index) {
  enum {numberOfTries=3};
  unsigned version = 0;
  unsigned failCount = 0;
  printf("\n\t--flush-%u-", index);
  while ((failCount++<numberOfTries)
      && (Failure == _pgp->readRegister(&_d, VersionAddr, 0x5e4+index, &version))) {
    printf("%s(%u)-", _d.name(), failCount);
  }
  if (failCount<numberOfTries) {
    printf("%s version(0x%x)\n\t", _d.name(), version);
    return false;
  } else {
    if (index > 2) {
      printf("_robustReadVersion FAILED!!\n\t");
      return true;
    } else {
      _pgp->resetPgpLane();
      microSpin(10);
      return _robustReadVersion(++index);
    }
  }
  return false;
}

unsigned Epix10kaConfigurator::configure( Epix10kaConfigType* c, unsigned first) {
  _config = c;
  _s = (Epix10kaConfigShadow*) c;
  _s->set(Epix10kaConfigShadow::UsePgpEvr, _fiberTriggering ? 1 : 0);
  char* str = (char*) calloc( 256, sizeof(char));
  timespec      start, end;
  bool printFlag = true;
  if (printFlag) printf("Epix10ka Config size(%u)", _config->_sizeof());
  printf(" config(%p) first(%u)\n", _config, first);
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  //  _pgp->maskHWerror(true);
  printf("Epix10kaConfigurator::configure %sreseting front end\n", first ? "" : "not ");
  if (first) {
    resetFrontEnd();
    _first = true;
      usleep(250000);
  }
  ret |= _robustReadVersion(0);
  ret |= this->G3config(c);
  uint32_t returned = 0;
  _pgp->readRegister(&_d, AdcControlAddr, 0x5e9, &returned);
  if ((returned & AdcCtrlFailMask) != 0) {
    sprintf(str, "Epix10kaConfigurator::configure found that ADC alignment FAILED!!! returned %d\n", returned);
    _pgp->errorStringAppend(str);
    printf("%s", str);
    ret |= 1;
  } else {
    printf("Epix10kaConfigurator::configure found ADC alignment Succeeded, returned %d\n", returned);
  }
  if (ret == 0) {
    resetSequenceCount();
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
    }
    if (printFlag) printf("\n\twriting top level config");
    ret <<= 1;
    enableRunTrigger(false);
    ret |= writeConfig();
    enableRunTrigger(true);
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    if (printFlag) printf("\n\twriting ASIC regs");
    enableRunTrigger(false);
    ret |= writeASIC();
    loadRunTimeConfigAdditions(_runTimeConfigFileName);
    enableRunTrigger(true);
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\t", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    if (usleep(10000)<0) perror("Epix10kaConfigurator::configure second ulseep failed\n");
  }
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone \n", ret);
    printf(" it took %lld.%lld milliseconds with first %u\n", diff/1000000LL, diff%1000000LL, first);
    if (ret) dumpFrontEnd();
  }
  return ret;
}

unsigned Epix10kaConfigurator::G3config(Epix10kaConfigType* c) {
  unsigned ret = 0;
  unsigned runTick = Pds_ConfigDb::EventcodeTiming::timeslot((unsigned)c->evrRunCode());
  unsigned daqTick = Pds_ConfigDb::EventcodeTiming::timeslot((unsigned)c->evrDaqCode());
  unsigned daqDelay = 0;
  if (runTick == daqTick) {
    daqDelay = (unsigned)c->evrRunTrigDelay() + 15;
  } else if (runTick > daqTick) {
    daqDelay = (runTick - daqTick) + (unsigned)c->evrRunTrigDelay() + 15;
  } else if (daqTick > runTick) {
    if ((daqTick - runTick) < ((unsigned)c->evrRunTrigDelay() + 64)) {
      daqDelay = ((unsigned)c->evrRunTrigDelay() + 64) + runTick - daqTick;
    } else {
      printf("Epix10kaConfigurator::G3config Timing error!!!!!\n");
      return 1;
    }
  }

  _d.dest(Epix10kaDestination::Registers);
  if (_pgp->G3Flag()) {
    if ((fiberTriggering() != 0)) {
      if (evrEnabled() == false) {
        evrEnable(true);
      }
      if (evrEnabled(true) == true) {
        ret |= evrEnableHdrChk(Epix10ka::Epix10kaDestination::Data, true);
        ret |= evrRunCode((unsigned)c->evrRunCode());
        ret |= evrRunDelay((unsigned)c->evrRunTrigDelay());
        ret |= evrDaqCode((unsigned)c->evrDaqCode());
        ret |= evrDaqDelay(daqDelay);
        ret |=  waitForFiducialMode(false);
        microSpin(10);
        ret |= evrLaneEnable(false);
        microSpin(10);
        ret |=  waitForFiducialMode(true);
        printf("Epix10kaConfigurator::G3config setting up fiber triggering on G3 pgpcard\n");
        _pgp->writeRegister(&_d, PgpTriggerEnable, 1);
      } else {
        ret = 1;
      }
    } else {
      ret |= evrEnableHdrChk(Epix10ka::Epix10kaDestination::Data, false);
      ret |= waitForFiducialMode(false);
      _pgp->writeRegister(&_d, PgpTriggerEnable, 0);
      printf("Epix10kaConfigurator::G3config setting up TTL triggering on G3 pgpcard\n");
    }
  } else {
    _pgp->writeRegister(&_d, PgpTriggerEnable, 0);
    printf("Epix10kaConfigurator::G3config setting up TTL triggering on G2 pgpcard\n");
  }
  return ret;
}

static uint32_t FPGAregs[6][3] = {
    {PowerEnableAddr, PowerEnableValue, 0},
    //    {SaciClkBitAddr, SaciClkBitValue, 0},
    //    {NumberClockTicksPerRunTriggerAddr, NumberClockTicksPerRunTrigger, 0},  // remove when in config
    //    {EnableAutomaticRunTriggerAddr, 0, 0},  // remove when in config
    {EnableAutomaticDaqTriggerAddr, 0, 0},
    {0,0,1}
};

char idNames[6][40] = {
    {"DigitalCardId0"},
    {"DigitalCardId1"},
    {"AnalogCardId0"},
    {"AnalogCardId1"},
    {"CarrierId0"},
    {"CarrierId1"}
};

unsigned Epix10kaConfigurator::writeConfig() {
  _d.dest(Epix10kaDestination::Registers);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  uint32_t tot = 0x8580; // as per Maciej
  unsigned ret = Success;
  unsigned i=0;
  unsigned idn = 0;
  printf("\n");
  while ((ret == Success) && (FPGAregs[i][2] == 0)) {
    if (_debug & 1) printf("Epix10kaConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", FPGAregs[i][0], FPGAregs[i][1], i);
    if (_pgp->writeRegister(&_d, FPGAregs[i][0], FPGAregs[i][1])) {
      printf("Epix10kaConfigurator::writeConfig failed writing FPGAregs[%u]\n", i);
      ret = Failure;
    }
    i+=1;
  }
  printf("Epix10kaConfigurator::writeConfig: writing 2 to 200\n");
  if (_pgp->writeRegister(&_d, 0x200, 2)) {
    printf("Epix10kaConfigurator::writeConfig failed writing %s\n", "0x200");
    ret = Failure;
  }

  printf("Epix10kaConfigurator::writeConfig: total pixels to read %u, \n",
      /*PixelsPerBank * (_s->get(Epix10kaConfigShadow::NumberOfReadableRowsPerAsic)+
          _config->calibrationRowCountPerASIC())*/ tot);
  if (_pgp->writeRegister(&_d, TotalPixelsAddr, tot)) {
    printf("Epix10kaConfigurator::writeConfig failed writing %s\n", "TotalPixelsAddr");
    ret = Failure;
  }
  for (unsigned i=0; !ret && i<(Epix10kaConfigShadow::NumberOfValues); i++) {
    if ((configAddrs[i][1] == ReadWrite) || (configAddrs[i][1] == WriteOnly)) {
      if (_pgp->writeRegister(&_d, configAddrs[i][0], u[i])) {
        printf("Epix10kaConfigurator::writeConfig failed writing %s\n", _s->name((Epix10kaConfigShadow::Registers)i));
        ret = Failure;
      } else {
        if (_debug & 1) printf("Epix10kaConfigurator::writeConfig writing %s addr(0x%x) data(0x%x) configValue(%u)\n",
            _s->name((Epix10kaConfigShadow::Registers)i), configAddrs[i][0], u[i], i);
      }
    } else if (configAddrs[i][1] == ReadOnly) {
      if (_debug & 1) printf("Epix10kaConfigurator::writeConfig reading addr(0x%x)", configAddrs[i][0]);
      if (_pgp->readRegister(&_d, configAddrs[i][0], 0x1000+i, u+i)) {
        printf("Epix10kaConfigurator::writeConfig failed reading %s\n", _s->name((Epix10kaConfigShadow::Registers)i));
        ret |= Failure;
      }
      if (_debug & 1) printf(" data(0x%x) configValue[%u]\n", u[i], i);
      if (((configAddrs[i][0]>>4) == 3) && (configAddrs[i][0] != 0x3a)) {
        printf("\t%s\t0x%x\n", idNames[idn++], u[i]);
      }
    }
    if (_pgp->writeRegister(&_d, DaqTrigggerDelayAddr, RunToDaqTriggerDelay+_s->get(Epix10kaConfigShadow::EpixRunTrigDelay))) {
      printf("Epix10kaConfigurator::writeConfig failed writing DaqTrigggerDelay\n");
      ret = Failure;
    }
    microSpin(100);
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned Epix10kaConfigurator::checkWrittenConfig(bool writeBack) {
  unsigned ret = Success;
  unsigned size = Epix10kaConfigShadow::NumberOfValues;
  uint32_t myBuffer[size];
  if (_s && _pgp) {
    _d.dest(Epix10kaDestination::Registers);
    for (unsigned i=0;_pgp && (i<size); i++) {
      if ((configAddrs[i][1] == ReadWrite) && _pgp) {
        if (_pgp->readRegister(&_d, configAddrs[i][0], (0x1100+i), myBuffer+i)) {
          printf("Epix10kaConfigurator::checkWrittenConfig failed reading %s\n", _s->name((Epix10kaConfigShadow::Registers)i));
          ret |= Failure;
        }
      }
    }
  }
  if (_s && _pgp) {
    if (ret == Success) {
      Epix10kaConfigShadow* readConfig = (Epix10kaConfigShadow*) myBuffer;
      uint32_t r, c;
      for (unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
        if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::ReadWrite) {
          if ((r=readConfig->get((Epix10kaConfigShadow::Registers)i)) !=
              (c=_s->get((Epix10kaConfigShadow::Registers)i))) {
            printf("Epix10kaConfigurator::checkWrittenConfig failed, rw(0x%x), config 0x%x!=0x%x readback at %s. %sriting back to config.\n",
                configAddrs[i][1], c, r, _s->name((Epix10kaConfigShadow::Registers)i), writeBack ? "W" : "Not w");
            if (writeBack) _s->set((Epix10kaConfigShadow::Registers)i, r);
            ret |= Failure;
          }
        }
      }
    }
  }
  if (!_pgp) printf("Epix10kaConfigurator::checkWrittenConfig found nil pgp\n");
  return ret;
}

//static uint32_t maskLineRegs[5][3] = {
//    {0x0,    0x0, 0},
//    {0x8000, 0x0, 0},
//    {0x6011, 0x0, 0},
//    {0x2000, 0x2, 0},
//    {0x0,    0x0, 1}
//};

unsigned Epix10kaConfigurator::writeASIC() {
  unsigned ret = Success;
  _d.dest(Epix10kaDestination::Registers);
  unsigned m = _config->asicMask();
  uint32_t* u = (uint32_t*)_config;
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      u = (uint32_t*) &_config->asics(index);
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      for (unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfValues; i++) {
        if ((AconfigAddrs[i][1] == ReadWrite) || (AconfigAddrs[i][1] == WriteOnly)) {
          if (_debug & 1) printf("Epix10kaConfigurator::writeAsicConfig writing addr(0x%x) data(0x%x) configValue(%u) Asic(%u)\n",
              a+AconfigAddrs[i][0], u[i], i, index);
          if (_pgp->writeRegister( &_d, a+AconfigAddrs[i][0], u[i], false, Pgp::PgpRSBits::Waiting)) {
            printf("Epix10kaConfigurator::writeASIC failed on %s, ASIC %u\n",
                Epix10kaASIC_ConfigShadow::name((Epix10kaASIC_ConfigShadow::Registers)i), index);
            ret |= Failure;
          }
          _getAnAnswer();
        } else if (AconfigAddrs[i][1] == ReadOnly) {
          if (_debug & 1) printf("Epix10kaConfigurator::writeAsic reading addr(0x%x)", a+AconfigAddrs[i][0]);
          if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0x1100+i, u+i)) {
            printf("Epix10kaConfigurator::writeASIC failed reading 0x%x\n", a+AconfigAddrs[i][0]);
            ret |= Failure;
          }
          if (_debug & 1) printf(" data(0x%x) configValue[%u] Asic(%u)\n", u[i], i, index);
        }
      }
    }
  }
  ret |= writePixelBits();
  if (ret==Success) return checkWrittenASIC(true);
  else return ret;
}

unsigned Epix10kaConfigurator::writePixelBits() {
  enum { writeAhead = 18 };
  unsigned ret = Success;
  _d.dest(Epix10kaDestination::Registers);
  unsigned writeCount = 0;
  unsigned m    = _config->asicMask();
  unsigned rows = _config->numberOfRows();
  unsigned cols = _config->numberOfColumns();
  unsigned synchDepthHisto[writeAhead];
  unsigned pops[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned totPixels = 0;
  // find the most popular pixel setting
  for (unsigned r=0; r<rows; r++) {
    for (unsigned c=0; c<cols; c++) {
      pops[_config->asicPixelConfigArray()(r,c)&15] += 1;
      totPixels += 1;
    }
  }
  unsigned max = 0;
  uint32_t pixel = _config->asicPixelConfigArray()(0,0) & 0xffff;
  printf("\nEpix10kaConfigurator::writePixelBits() histo");
  for (unsigned n=0; n<16; n++) {
    printf(" %u,", pops[n]);
    if (pops[n] > max) {
      max = pops[n];
      pixel = n;
    }
  }
  printf(" pixel %u, total pixels %u\n", pixel, totPixels);
  if (_first) {
    _first = false;
    if ((totPixels != pops[pixel]) || (pixel)) {
      usleep(1400000);
      printf("Epix10kaConfigurator::writePixelBits sleeping 1.4 seconds\n");
    }
  }

  //  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 4, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
  //    printf("Epix10kaConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
  //    ret |= Failure;
  //  }
  // program the entire array to the most popular setting
  // note, that this is necessary because the global pixel write does not work in this device and programming
  //   one pixel in one bank also programs the same pixel in the other banks
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareMultiConfigAddr, 0, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
        printf("Epix10kaConfigurator::writePixelBits failed on ASIC %u, writing prepareMultiConfig\n", index);
        ret |= Failure;
      }
      writeCount += 1;
    }
  }
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+WriteWholeMatricAddr, pixel, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
        printf("Epix10kaConfigurator::writePixelBits failed on ASIC %u, writing entire matrix\n", index);
        ret |= Failure;
      }
      writeCount += 1;
    }
  }

  //  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
  //    if (m&(1<<index)) {
  //      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
  //      for (unsigned col=0; col<PixelsPerBank; col++) {
  //        if (_pgp->writeRegister( &_d, a+ColCounterAddr, col, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
  //          printf("Epix10kaConfigurator::writePixelBits failed on ASIC %u, writing ColCounter %u\n", index, col);
  //          ret |= Failure;
  //        }
  //        if (_pgp->writeRegister( &_d, a+WriteColDataAddr, pixel, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
  //          printf("Epix10kaConfigurator::writePixelBits failed on ASIC %u, writing Col Data %u\n", index, col);
  //          ret |= Failure;
  //        }
  //        _getAnAnswer();
  //      }
  //    }
  //  }
  // now program the ones that were not the same
  //  NB must program four at a time because that's how the asic works
  //  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 3, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
  //    printf("Epix10kaConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
  //    ret |= Failure;
  //  }
  unsigned offset = 0;
  unsigned bank = 0;
  unsigned bankOffset = 0;
  unsigned banks[4] = {Bank0, Bank1, Bank2, Bank3};
  unsigned myRow = 0;
  unsigned myCol = 0;
  unsigned bankHisto[4] = {0,0,0,0};
  unsigned asicHisto[4] = {0,0,0,0};
  uint32_t thisPix = 0;
  // allow it queue up writeAhead commands
  Pds::Pgp::ConfigSynch mySynch(_fd, writeAhead, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t));
  for (unsigned row=0; row<rows; row++) {
    for (unsigned col=0; col<cols; col++) {
      if ((thisPix = (_config->asicPixelConfigArray()(row,col)&15)) != pixel) {
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
        if (m&(1<<offset)) {
          bank = (myCol % (PixelsPerBank<<2)) / PixelsPerBank;
          bankOffset = banks[bank];
          bankHisto[bank]+= 1;
          asicHisto[offset] += 1;
          if (mySynch.take() == false) {
            printf("Epix10kaConfigurator::writePixelBits synchronization failed on asic %u write of calib row %u column %u\n",
                offset, myRow, myCol);
            ret |= Failure;
          }
//          printf("Epix10kaConfigurator::writePixelBits asic %d, row %d col %d\n", offset, myRow, myCol);
          if (_pgp->writeRegister( &_d,
              RowCounterAdder + AsicAddrBase + AsicAddrOffset * offset, myRow,
              (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting)) {
            printf("Epix10kaConfigurator::writePixelBits failed on asic %u, pixel row %u, col %u\n", offset, myRow, myCol);
            ret |= Failure;
          } else {
            if (mySynch.take() == false) {
              printf("Epix10kaConfigurator::writePixelBits synchronization failed asic %u on write of calib row %u column %u\n",
                  offset, myRow, myCol);
              ret |= Failure;
            }
            if (_pgp->writeRegister(&_d,
                ColCounterAddr + AsicAddrBase + AsicAddrOffset * offset, bankOffset | (myCol % PixelsPerBank),
                (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting)) {
              printf("Epix10kaConfigurator::writePixelBits failed on pixel row %u, col %u\n", row, col);
              ret |= Failure;
            } else {
              if (mySynch.take() == false) {
                printf("Epix10kaConfigurator::writePixelBits synchronization failed on asic %u write of calib row %u column %u\n",
                    offset, row, col);
                ret |= Failure;
              }
              if (_pgp->writeRegister( &_d,
                  PixelDataAddr + AsicAddrBase + AsicAddrOffset * offset, thisPix,
                  (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting))  {

                printf("Epix10kaConfigurator::writePixelBits failed on asic %u pixel row %u, col %u\n", offset, row, col);
                ret |= Failure;
              }
            }
          }
          writeCount += 1;
        }
      }
      if (ret & Failure) {
        printf("Epix10kaConfigurator::writePixelBits failed on row %u, col %u\n", row, col);
        break;
      }
    }
    if (ret & Failure) {
      break;
    }
  }

  //  there is one pixel configuration row per each pair of calibration rows.
  unsigned calibRow = 176;
  for (unsigned row=0; row<2; row++) {
    printf("Epix10kaConfigurator::writePixelBits calibration row 0x%x\n", row);
    for (unsigned col=0; col<cols; col++) {
      pixel = _config->calibPixelConfigArray()(row,col) & 3;
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
      if (m&(1<<offset)) {
//        if (0) {
        bank = (myCol % (PixelsPerBank<<2)) / PixelsPerBank;
        bankOffset = banks[bank];
        if (mySynch.take() == false) {
          printf("Epix10kaConfigurator::writePixelBits synchronization failed on write of calib row %u column %u\n",
              row, col);
          ret |= Failure;
        }
        if (_pgp->writeRegister( &_d,
            RowCounterAdder + AsicAddrBase + AsicAddrOffset * offset, calibRow,
            (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting)) {
          printf("Epix10kaConfigurator::writePixelBits failed on asic %u, calib row %u, col %u\n", offset, row, myCol);
          ret |= Failure;
        } else {
          if (mySynch.take() == false) {
            printf("Epix10kaConfigurator::writePixelBits synchronization failed on write of calib row %u column %u\n",
                row, col);
            ret |= Failure;
          }
          if (_pgp->writeRegister(&_d,
              ColCounterAddr + AsicAddrBase + AsicAddrOffset * offset, bankOffset | (myCol % PixelsPerBank),
              (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting)) {
            printf("Epix10kaConfigurator::writePixelBits failed on asic %u, calib row %u, col %u\n", offset, row, myCol);
            ret |= Failure;
          } else {
            if (mySynch.take() == false) {
              printf("Epix10kaConfigurator::writePixelBits synchronization failed on write of calib row %u column %u\n",
                  row, col);
              ret |= Failure;
            }
            if (_pgp->writeRegister( &_d,
                PixelDataAddr + AsicAddrBase + AsicAddrOffset * offset, pixel,
                (_debug&1)?true:false, Pds::Pgp::PgpRSBits::Waiting))  {

              printf("Epix10kaConfigurator::writePixelBits failed on asic, %u, calib row %u, col %u\n", offset, row, myCol);
              ret |= Failure;
            }
            //      }
          }
        }
        if (ret & Failure) {
          break;
        }
        writeCount += 3;
      }
    }
    if (ret & Failure) {
      break;
    }
  }
  if (mySynch.clear() == false) {
    printf("Epix10kaConfigurator::writePixelBits synchronization failed to clear\n");
  }
  printf("\nEpix10kaConfigurator::writePixelBits write count %u\n", writeCount);
  printf("Epix10kaConfigurator::writePixelBits PrepareForReadout for ASIC:");
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareForReadout, 0, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
        printf("Epix10kaConfigurator::writePixelBits failed on ASIC %u\n", index);
        ret |= Failure;
      }
      printf(" %u", index);
      _getAnAnswer();
    }
  }
  printf("\n");
  printf("Epix10kaConfigurator::writePixelBits banks %u %u %u %u\n",
      bankHisto[0], bankHisto[1],bankHisto[2],bankHisto[3]);
  printf("Epix10kaConfigurator::writePixelBits asics %u %u %u %u\n",
      asicHisto[0], asicHisto[1],asicHisto[2],asicHisto[3]);
  //  if (_pgp->writeRegister( &_d, SaciClkBitAddr, SaciClkBitValue, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
  //    printf("Epix10kaConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
  //    ret |= Failure;
  //  }
  return ret;
}


bool Epix10kaConfigurator::_getAnAnswer(unsigned size, unsigned count) {
  Pds::Pgp::RegisterSlaveImportFrame* rsif;
  count = 0;
  while ((count < 6) && (rsif = _pgp->read(size)) != 0) {
    if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting) {
      //      printf("_getAnAnswer in %u tries\n", count+1);
      return true;
    }
    count += 1;
  }
  return false;
}

unsigned Epix10kaConfigurator::checkWrittenASIC(bool writeBack) {
  unsigned ret = Success;
//  return ret;
  bool done = false;
  while (!done && _config && _pgp) {
    printf("Epix10kaConfigurator::checkWrittenASIC ");
    _d.dest(Epix10kaDestination::Registers);
    unsigned m = _config->asicMask();
    uint32_t myBuffer[sizeof(Epix10kaASIC_ConfigShadow)/sizeof(uint32_t)];
    Epix10kaASIC_ConfigShadow* readAsic = (Epix10kaASIC_ConfigShadow*) myBuffer;
    for (unsigned index=0; index<_config->numberOfAsics(); index++) {
      if (m&(1<<index)) {
        printf("reading(%u)->", index);
        uint32_t a = AsicAddrBase + (AsicAddrOffset * index);
        for (int i = 0; (i<Epix10kaASIC_ConfigShadow::NumberOfValues) && (ret==Success); i++) {
          if ((AconfigAddrs[i][1] != WriteOnly) && _pgp) {
            if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0xa000+(index<<2)+i, myBuffer+i, 1)){
              printf("Epix10kaConfigurator::checkWrittenASIC read reg %u failed on ASIC %u at 0x%x\n",
                  i, index, a+AconfigAddrs[i][0]);
              return Failure;
            }
            myBuffer[i] &= 0xffff;
          }
        }
        printf("checking, ");
        Epix10kaASIC_ConfigShadow* confAsic = (Epix10kaASIC_ConfigShadow*) &( _config->asics(index));
        if ( (ret==Success) && (*confAsic != *readAsic) && _pgp) {
          printf("Epix10kaConfigurator::checkWrittenASIC failed on ASIC %u\n", index);
//          ret = Failure;
          if (writeBack) *confAsic = *readAsic;
        }
      }
    }
    printf("\n");
    done = true;
  }
  if (!_pgp) printf("Epix10kaConfigurator::checkWrittenASIC found nil pgp\n");
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

void Epix10kaConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
//    dumpPgpCard();
    uint32_t count = 0x1111;
    uint32_t acount = 0x1112;
    count = sequenceCount();
    acount = acquisitionCount();
    printf("\tSequence Count(%u), Acquisition Count(%u)\n", count, acount);
    printf("Environmental Data:\n");
    for (int i=0; i<NumberOfEnviroDatas; i++) {
      if (i<3) printf("\t%s%5.2f\n", enviroNames[i], 0.01*(signed int)enviroData(i));
      else printf("\t%s%5d\n", enviroNames[i], enviroData(i));
    }
  }
  if (_debug & 0x400) {
    //    printf("Checking Configuration, no news is good news ...\n");
    //    if (Failure == checkWrittenConfig(false)) {
    //      printf("Epix10kaConfigurator::checkWrittenConfig() FAILED !!!\n");
    //    }
    //    enableRunTrigger(false);
    //    if (Failure == checkWrittenASIC(false)) {
    //      printf("Epix10kaConfigurator::checkWrittenASICConfig() FAILED !!!\n");
    //    }
    //    enableRunTrigger(true);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("Epix10kaConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
