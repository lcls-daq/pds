/*
 * Epix100aConfigurator.cc
 *
 *  Created on: 2014.7.31
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
#include "pds/config/EpixConfigType.hh"
#include "pds/pgp/Configurator.hh"
#include "pds/epix100a/Epix100aConfigurator.hh"
#include "pds/epix100a/Epix100aDestination.hh"
#include "pds/epix100a/Epix100aStatusRegisters.hh"
#include "pds/config/EventcodeTiming.hh"
#include "ndarray/ndarray.h"

using namespace Pds::Epix100a;

class Epix100aDestination;

//  configAddrs array elements are address:useModes

static uint32_t configAddrs[Epix100aConfigShadow::NumberOfValues][2] = {
    {0x00,  1}, //  version
    {0x3d,  0}, //  usePgpEvr
    {0,		2}, //  evrRunCode
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
    {0x2c,  0}, //  AdcPipelineDelay
    {0x90,  0}, //  AdcPipelineDelay0
    {0x91,  0}, //  AdcPipelineDelay1
    {0x92,  0}, //  AdcPipelineDelay2
    {0x93,  0}, //  AdcPipelineDelay3
    {0x2d,  0}, //  SyncParams
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

static uint32_t AconfigAddrs[Epix100aASIC_ConfigShadow::NumberOfValues][2] = {
    {0x1001,  0},  //  pulserVsPixelOnDelay and pulserSync
    {0x1002,  0},  //  dummy pixel
    {0x1003,  0},  //
    {0x1004,  0},  //
    {0x1005,  0},  //
    {0x1006,  0},  //
    {0x1007,  0},  //
    {0x1008,  0},  //
    {0x1009,  0},  //
    {0x100a,  0},  //
    {0x100b,  0},  //
    {0x100c,  0},  //
    {0x100d,  0},  //
    {0x100e,  0},  //  
    {0x100f,  0},  //  
    {0x1010,  0},  //  
    {0x1011,  3},  //  RowStartAddr
    {0x1012,  3},  //  RowStopAddr
    {0x1013,  3},  //  ColStartAddr
    {0x1014,  3},  //  ColStopAddr
    {0x1015,  1},  //  chipID
    {0x1016,  0},  //
    {0x1017,  0},  //
    {0x1018,  0},  //
    {0x1019,  0}   //
};


Epix100aConfigurator::Epix100aConfigurator(bool use_aes, int f, unsigned d) :
                               Pds::Pgp::Configurator(use_aes, f, d),
                               _testModeState(0), _config(0), _s(0), _rhisto(0),
                               _maintainLostRunTrigger(0) {
  allocateVC(7);
  _d.dest(Epix100aDestination::Registers);  // Turn on monitoring immediately.
  _pgp->writeRegister(&_d, 1, 1);
  _pgp->writeRegister(&_d, MonitorEnableAddr, 1);
  printf("Epix100aConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

Epix100aConfigurator::~Epix100aConfigurator() {
  evrLaneEnable(false);
}

unsigned Epix100aConfigurator::resetFrontEnd() {
  unsigned count = 0;
  unsigned monitorEnable = 0;
  _d.dest(Epix100aDestination::Registers);
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
    printf("Epix100aConfigurator::resetFrontEnd found that ADC alignment FAILED!!! returned %d\n", returned);
    return resyncADC(1);
  }
  _pgp->writeRegister(&_d, MonitorEnableAddr, monitorEnable);
  printf("Epix100aConfigurator::resetFrontEnd found ADC alignment Succeeded, returned %d\n", returned);
  return 0;
}

unsigned Epix100aConfigurator::resyncADC(unsigned c) {
  unsigned ret = 0;
  unsigned count = 0;
  _d.dest(Epix100aDestination::Registers);
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
    printf("\tEpix100aConfigurator::resyncADC found that ADC alignment FAILED!!! returned %d\n", returned);
    if (c>0) ret = resyncADC(c-1);
    else ret = 1;
  } else {
    printf("\tEpix100aConfigurator::resyncADC found ADC alignment Succeeded, returned %d\n", returned);
  }
  return ret;
}

void Epix100aConfigurator::resetSequenceCount() {
  _d.dest(Epix100aDestination::Registers);
  if (_pgp) {
    if(fiberTriggering()) {
      _pgp->resetSequenceCount();
    }
    _pgp->writeRegister(&_d, ResetFrameCounter, 1);
    _pgp->writeRegister(&_d, ResetAcqCounter, 1);
    microSpin(10);
  } else {
    printf("Epix100aConfigurator::resetSequenceCount() found nil _pgp so not reset\n");
  }
}

uint32_t Epix100aConfigurator::sequenceCount() {
  _d.dest(Epix100aDestination::Registers);
  uint32_t count=1111;
  if (_pgp) _pgp->readRegister(&_d, ReadFrameCounter, 0x5e4, &count);
  else printf("Epix100aConfigurator::sequenceCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Epix100aConfigurator::acquisitionCount() {
  _d.dest(Epix100aDestination::Registers);
  uint32_t count=1112;
  if (_pgp) _pgp->readRegister(&_d, ReadAcqCounter, 0x5e4, &count);
  else printf("Epix100aConfigurator::acquisitionCount() found nil _pgp so not read\n");
  return (count);
}

uint32_t Epix100aConfigurator::enviroData(unsigned o) {
  _d.dest(Epix100aDestination::Registers);
  uint32_t dat=1113;
  if (_pgp) _pgp->readRegister(&_d, EnviroDataBaseAddr+o, 0x5e4, &dat);
  else printf("Epix100aConfigurator::enviroData() found nil _pgp so not read\n");
  return (dat);
}

void Epix100aConfigurator::enableExternalTrigger(bool f) {
  _d.dest(Epix100aDestination::Registers);
  if (_pgp) {
    _pgp->writeRegister(&_d, DaqTriggerEnable, f ? Enable : Disable);
  } else printf("Epix100aConfigurator::enableExternalTrigger(%s) found nil _pgp so not set\n", f ? "true" : "false");
}

void Epix100aConfigurator::enableRunTrigger(bool f) {
  unsigned mask = 1<<(_pgp->portOffset());
  _d.dest(Epix100aDestination::Registers);
  _pgp->writeRegister(&_d, RunTriggerEnable, f ? Enable : Disable);
  _pgp->maskRunTrigger(!f);
  printf("Epix100aConfigurator::enableRunTrigger(%s), mask 0x%x\n", f ? "true" : "false", mask);
}

void Epix100aConfigurator::print() {}

void Epix100aConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

void Epix100aConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("Epix100aConfigurator::runTimeConfigName(%s)\n", name);
}

bool Epix100aConfigurator::_robustReadVersion(unsigned index) {
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

unsigned Epix100aConfigurator::configure( Epix100aConfigType* c, unsigned first) {
  _config = c;
  _s = (Epix100aConfigShadow*) c;
  _s->set(Epix100aConfigShadow::UsePgpEvr, _fiberTriggering ? 1 : 0);
  unsigned debugSaved = _debug;
  char* str = (char*) calloc( 256, sizeof(char));
  _debug = 0;
  timespec      start, end;
  bool printFlag = true;
  if (printFlag) printf("Epix100a Config size(%u)", _config->_sizeof());
  printf(" config(%p) first(%u)\n", _config, first);
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  //  _pgp->maskHWerror(true);
  printf("Epix100aConfigurator::configure %sreseting front end\n", first ? "" : "not ");
  if (first) {
    resetFrontEnd();
  }
  ret |= _robustReadVersion(0);
  ret |= this->G3config(c);
  uint32_t returned = 0;
  _pgp->readRegister(&_d, AdcControlAddr, 0x5e9, &returned);
  if ((returned & AdcCtrlFailMask) != 0) {
    sprintf(str, "Epix100aConfigurator::configure found that ADC alignment FAILED!!! returned %d\n", returned);
    _pgp->errorStringAppend(str);
    printf("%s", str);
    ret |= 1;
  } else {
    printf("Epix100aConfigurator::configure found ADC alignment Succeeded, returned %d\n", returned);
  }
  if (ret == 0) {
    resetSequenceCount();
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    enableRunTrigger(false);
    if (printFlag) printf("\n\twriting top level config\n");
    ret |= writeConfig();
    enableRunTrigger(true);
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    enableRunTrigger(false);
    if (printFlag) printf("\n\twriting ASIC regs\n");
    ret |= writeASIC();
    loadRunTimeConfigAdditions(_runTimeConfigFileName);
    enableRunTrigger(true);
    if (printFlag) {
      clock_gettime(CLOCK_REALTIME, &end);
      uint64_t diff = timeDiff(&end, &start) + 50000LL;
      printf("- 0x%x - so far %lld.%lld milliseconds\n", ret, diff/1000000LL, diff%1000000LL);
    }
    ret <<= 1;
    if (usleep(10000)<0) perror("Epix100aConfigurator::configure second ulseep failed\n");
  }
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("- 0x%x - \n\tdone \n", ret);
    printf(" it took %lld.%lld milliseconds with first %u\n", diff/1000000LL, diff%1000000LL, first);
    if (ret) dumpFrontEnd();
  }
  _debug = debugSaved;
  return ret;
}

unsigned Epix100aConfigurator::G3config(Epix100aConfigType* c) {
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
      printf("Epix100aConfigurator::G3config Timing error!!!!!\n");
      return 1;
    }
  }
  _d.dest(Epix100aDestination::Registers);
  if (_pgp->G3Flag()) {
    if (fiberTriggering()) {
      if (evrEnabled() == false) {
        evrEnable(true);
      }
      if (evrEnabled(true) == true) {
        ret |= evrEnableHdrChk(Epix100a::Epix100aDestination::Data, true);
        ret |= evrRunCode((unsigned)c->evrRunCode());
        ret |= evrRunDelay((unsigned)c->evrRunTrigDelay());
        ret |= evrDaqCode((unsigned)c->evrDaqCode());
        ret |= evrDaqDelay(daqDelay);
        ret |=  waitForFiducialMode(false);
        microSpin(10);
        ret |= evrLaneEnable(false);
        microSpin(10);
        ret |=  waitForFiducialMode(true);
        printf("Epix100aConfigurator::G3config setting up fiber triggering on G3 pgpcard\n");
        _pgp->writeRegister(&_d, PgpTriggerEnable, 1);
      } else {
        ret = 1;
      }
    } else {
      ret |= evrEnableHdrChk(Epix100a::Epix100aDestination::Data, false);
      ret |= waitForFiducialMode(false);
      _pgp->writeRegister(&_d, PgpTriggerEnable, 0);
      printf("Epix100aConfigurator::G3config setting up TTL triggering on G3 pgpcard\n");
    }
  } else {
    _pgp->writeRegister(&_d, PgpTriggerEnable, 0);
    printf("Epix100aConfigurator::G3config setting up TTL triggering on G2 pgpcard\n");
  }
  return ret;
}

static uint32_t FPGAregs[6][3] = {
    {PowerEnableAddr, PowerEnableValue, 0},
    {SaciClkBitAddr, SaciClkBitValue, 0},
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

unsigned Epix100aConfigurator::writeConfig() {
  _d.dest(Epix100aDestination::Registers);
  uint32_t* u = (uint32_t*)_config;
  _testModeState = *u;
  unsigned ret = Success;
  unsigned i=0;
  unsigned idn = 0;
  printf("\n");
  while ((ret == Success) && (FPGAregs[i][2] == 0)) {
    if (_debug & 1) printf("Epix100aConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", FPGAregs[i][0], FPGAregs[i][1], i);
    if (_pgp->writeRegister(&_d, FPGAregs[i][0], FPGAregs[i][1])) {
      printf("Epix100aConfigurator::writeConfig failed writing FPGAregs[%u]\n", i);
      ret = Failure;
    }
    i+=1;
  }
  if (_pgp->writeRegister(&_d, TotalPixelsAddr, PixelsPerBank *
      (_s->get(Epix100aConfigShadow::NumberOfReadableRowsPerAsic)+
          _config->calibrationRowCountPerASIC()))) {
    printf("Epix100aConfigurator::writeConfig failed writing %s\n", "TotalPixelsAddr");
    ret = Failure;
  }
  for (unsigned i=0; !ret && i<(Epix100aConfigShadow::NumberOfValues); i++) {
    if (configAddrs[i][1] == ReadWrite) {
      if (_pgp->writeRegister(&_d, configAddrs[i][0], u[i])) {
        printf("Epix100aConfigurator::writeConfig failed writing %s\n", _s->name((Epix100aConfigShadow::Registers)i));
        ret = Failure;
      } else {
        if (_debug & 1) printf("Epix100aConfigurator::writeConfig writing %s addr(0x%x) data(0x%x) configValue(%u)\n",
            _s->name((Epix100aConfigShadow::Registers)i), configAddrs[i][0], u[i], i);
      }
    } else if (configAddrs[i][1] == ReadOnly) {
      if (_debug & 1) printf("Epix100aConfigurator::writeConfig reading addr(0x%x)", configAddrs[i][0]);
      if (_pgp->readRegister(&_d, configAddrs[i][0], 0x1000+i, u+i)) {
        printf("Epix100aConfigurator::writeConfig failed reading %s\n", _s->name((Epix100aConfigShadow::Registers)i));
        ret |= Failure;
      }
      if (_debug & 1) printf(" data(0x%x) configValue[%u]\n", u[i], i);
      if (((configAddrs[i][0]>>4) == 3) && (configAddrs[i][0] != 0x3a)) {
        printf("\t%s\t0x%x\n", idNames[idn++], u[i]);
      }
    }
    if (_pgp->writeRegister(&_d, DaqTrigggerDelayAddr, RunToDaqTriggerDelay+_s->get(Epix100aConfigShadow::EpixRunTrigDelay))) {
      printf("Epix100aConfigurator::writeConfig failed writing DaqTrigggerDelay\n");
      ret = Failure;
    }
    microSpin(100);
  }
  if (ret == Success) return checkWrittenConfig(true);
  else return ret;
}

unsigned Epix100aConfigurator::checkWrittenConfig(bool writeBack) {
  unsigned ret = Success;
  unsigned size = Epix100aConfigShadow::NumberOfValues;
  uint32_t myBuffer[size];
  if (_s && _pgp) {
    _d.dest(Epix100aDestination::Registers);
    for (unsigned i=0;_pgp && (i<size); i++) {
      if ((configAddrs[i][1] == ReadWrite) && _pgp) {
        if (_pgp->readRegister(&_d, configAddrs[i][0], (0x1100+i), myBuffer+i)) {
          printf("Epix100aConfigurator::checkWrittenConfig failed reading %s\n", _s->name((Epix100aConfigShadow::Registers)i));
          ret |= Failure;
        }
      }
    }
  }
  if (_s && _pgp) {
    if (ret == Success) {
      Epix100aConfigShadow* readConfig = (Epix100aConfigShadow*) myBuffer;
      uint32_t r, c;
      for (unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
        if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::ReadWrite) {
          if ((r=readConfig->get((Epix100aConfigShadow::Registers)i)) !=
              (c=_s->get((Epix100aConfigShadow::Registers)i))) {
            printf("Epix100aConfigurator::checkWrittenConfig failed, rw(0x%x), config 0x%x!=0x%x readback at %s. %sriting back to config.\n",
                configAddrs[i][1], c, r, _s->name((Epix100aConfigShadow::Registers)i), writeBack ? "W" : "Not w");
            if (writeBack) _s->set((Epix100aConfigShadow::Registers)i, r);
            ret |= Failure;
          }
        }
      }
    }
  }
  if (!_pgp) printf("Epix100aConfigurator::checkWrittenConfig found nil pgp\n");
  return ret;
}

//static uint32_t maskLineRegs[5][3] = {
//    {0x0,    0x0, 0},
//    {0x8000, 0x0, 0},
//    {0x6011, 0x0, 0},
//    {0x2000, 0x2, 0},
//    {0x0,    0x0, 1}
//};

unsigned Epix100aConfigurator::writeASIC() {
  unsigned ret = Success;
  _d.dest(Epix100aDestination::Registers);
  unsigned m = _config->asicMask();
  uint32_t* u = (uint32_t*)_config;
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      u = (uint32_t*) &_config->asics(index);
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      for (unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfValues; i++) {
        if ((AconfigAddrs[i][1] == ReadWrite) || (AconfigAddrs[i][1] == WriteOnly)) {
          if (_debug & 1) printf("Epix100aConfigurator::writeAsicConfig writing addr(0x%x) data(0x%x) configValue(%u) Asic(%u)\n",
              a+AconfigAddrs[i][0], u[i], i, index);
          if (_pgp->writeRegister( &_d, a+AconfigAddrs[i][0], u[i], false, Pgp::PgpRSBits::Waiting)) {
            printf("Epix100aConfigurator::writeASIC failed on %s, ASIC %u\n",
                Epix100aASIC_ConfigShadow::name((Epix100aASIC_ConfigShadow::Registers)i), index);
            ret |= Failure;
          }
          _getAnAnswer();
        } else if (AconfigAddrs[i][1] == ReadOnly) {
          if (_debug & 1) printf("Epix100aConfigurator::writeAsic reading addr(0x%x)", a+AconfigAddrs[i][0]);
          if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0x1100+i, u+i)) {
            printf("Epix100aConfigurator::writeASIC failed reading 0x%x\n", a+AconfigAddrs[i][0]);
            ret |= Failure;
          }
          if (_debug & 1) printf(" data(0x%x) configValue[%u] Asic(%u)\n", u[i], i, index);
        }
      }
      //      unsigned j=0;
      //      while ((ret == Success) && (maskLineRegs[j][2] == 0)) {
      //        if (_debug & 1) printf("Epix100aConfigurator::writeConfig writing addr(0x%x) data(0x%x) FPGAregs[%u]\n", a+maskLineRegs[j][0], maskLineRegs[j][1], j);
      //        for (unsigned i=0; (ret == Success) && (i<RepeatControlCount); i++) {
      //          if (_pgp->writeRegister(&_d, a+maskLineRegs[j][0], maskLineRegs[j][1], false, Pgp::PgpRSBits::Waiting)) {
      //            printf("Epix100aConfigurator::writeConfig failed writing maskLineRegs[%u]\n", j);
      //            ret = Failure;
      //          }
      //          _getAnAnswer();
      //        }
      //        j+=1;
      //      }
    }
  }
  ret |= writePixelBits();
  if (ret==Success) return checkWrittenASIC(true);
  else return ret;
}

// from Kurtis
//  0x080000 : Row in global space (0-703)
//  0x080001 : Col in global space (0-767)
//  0x080002 : Left most pixel data
//  0x080003 : Second to left pixel data
//  0x080004 : Third to left pixel data
//  0x080005 : Right most pixel data

class block {
  public:
    block() : _row(0), _col(0){};
    ~block() {};
    void row( uint32_t r ) {_row = r; }
    uint32_t row() { return _row; }
    void col( uint32_t c ) {_col = c; }
    uint32_t col() { return _col; }
    uint32_t* b() { return _b; }
  private:
    uint32_t  _row;
    uint32_t  _col;
    uint32_t  _b[4];
};

// From Kurtis ...
//    1) set SaciClkBit to 4 (register 0x000028)
//    2) send ASIC command: PrepareMultiConfig (0x808000, 0x908000, 0xA08000, 0xB08000
//         for ASICs 0,1,2,3 respectively)  [data written doesn't matter]
//    3) loop over columns 0-95:
//       3a) write ASIC register: ColCounter (0x806013, 0x906013, 0xA06013, 0xB06013
//            for ASICs 0,1,2,3 respectively) [data is the column number]
//       3b) write ASIC register: WriteColData (0x803000, 0x903000, 0xA03000, 0xB03000
//            for ASICs 0,1,2,3 respectively) - [bits 1,0 are mask, test, respectively]
//    4) send ASIC command: PrepareForReadout: (0x800000, 0x900000, 0xA00000, 0xB00000)
//    5) (optional) SaciClkBit needs to be set to 3 to do individual pixel writes, so
//          you could set it back to 3 here, or incorporate that as part of the single
//          pixel write procedure
//
//Bit 16 of the row data to flag the write as targeting a calibration row.
//Bit 17 of the row data would indicate whether the row is on top (0) or bottom (1).
//So row on top you would do the same as the existing block write for 4x pixels, but with row: 0x00010000
//And row on bottom would do the same as the existing block write for 4x pixels, but with row: 0x00030000

unsigned Epix100aConfigurator::writePixelBits() {
  enum { writeAhead = 32 };
  unsigned ret = Success;
  _d.dest(Epix100aDestination::Registers);
  unsigned m    = _config->asicMask();
  unsigned rows = _config->numberOfRows();
  unsigned cols = _config->numberOfColumns();
  unsigned synchDepthHisto[writeAhead];
  bool written[rows][cols];
  unsigned pops[4] = {0,0,0,0};
  block blk;
  // find the most popular pixel setting
  for (unsigned r=0; r<rows; r++) {
    if (r<writeAhead) synchDepthHisto[r] = 0;
    for (unsigned c=0; c<cols; c++) {
      pops[_config->asicPixelConfigArray()(r,c)&3] += 1;
      written[r][c] = false;
    }
  }
  unsigned max = 0;
  uint32_t pixel = _config->asicPixelConfigArray()(0,0) & 0xffff;
  printf("\nEpix100aConfigurator::writePixelBits() histo");
  for (unsigned n=0; n<4; n++) {
    printf(" %u,", pops[n]);
    if (pops[n] > max) {
      max = pops[n];
      pixel = n;
    }
  }
  printf(" pixel %u\n", pixel);
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 4, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("Epix100aConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  // program the entire array to the most popular setting
  // note, that this is necessary because the global pixel write does not work in this device and programming
  //   one pixel in one bank also programs the same pixel in the other banks
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareMultiConfigAddr, 0, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
        printf("Epix100aConfigurator::writePixelBits failed on ASIC %u, writing prepareMultiConfig\n", index);
        ret |= Failure;
      }
    }
  }
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      for (unsigned col=0; col<PixelsPerBank; col++) {
        if (_pgp->writeRegister( &_d, a+ColCounterAddr, col, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
          printf("Epix100aConfigurator::writePixelBits failed on ASIC %u, writing ColCounter %u\n", index, col);
          ret |= Failure;
        }
        if (_pgp->writeRegister( &_d, a+WriteColDataAddr, pixel, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
          printf("Epix100aConfigurator::writePixelBits failed on ASIC %u, writing Col Data %u\n", index, col);
          ret |= Failure;
        }
        _getAnAnswer();
      }
    }
  }
  // now program the ones that were not the same
  //  NB must program four at a time because that's how the asic works
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, 3, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("Epix100aConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  unsigned halfRow = _config->numberOfColumns()/2;
  unsigned writeCount = 0;
  // allow it queue up writeAhead commands
  Pds::Pgp::ConfigSynch mySynch(_fd, writeAhead, this, sizeof(Pds::Pgp::RegisterSlaveExportFrame)/sizeof(uint32_t) + 5);
  for (unsigned row=0; row<rows; row++) {
    blk.row(row);
    for (unsigned col=0; col<cols; col++) {
      if ((!written[row][col]) && ((_config->asicPixelConfigArray()(row,col)&3) != pixel)) {
        blk.col(col);
        unsigned half = col < halfRow ? 0 : halfRow;
        for (unsigned i=0; i<BanksPerAsic; i++) {
          unsigned thisCol = half + (i * PixelsPerBank) + (col % PixelsPerBank);
          written[row][thisCol] = true;
          blk.b()[i] = _config->asicPixelConfigArray()(row,thisCol) & 3;
        }
        if (mySynch.take() == false) {
          printf("Epix100aConfigurator::writePixelBits synchronization failed on write %u\n", writeCount);
          ret |= Failure;
        }
        if (_pgp->writeRegisterBlock(
            &_d,
            MultiplePixelWriteCommandAddr,
            (unsigned*)&blk,
            sizeof(block)/sizeof(uint32_t),
            Pds::Pgp::PgpRSBits::Waiting,
            (_debug&1)?true:false))
        {
          printf("Epix100aConfigurator::writePixelBits failed on row %u, col %u\n", row, col);
          ret |= Failure;
        }
        synchDepthHisto[mySynch.depth()] += 1;
        writeCount += 1;
      }
    }
  }
  printf("Epix100aConfigurator::writePixelBits write count %u\n", writeCount);
  // NB we are dividing the calibration rows by two because
  //  there is one pixel configuration row per each pair of calibration rows.
  for (unsigned row=0; row<(_config->numberOfCalibrationRows()/2); row++) {
    blk.row(row ? BottomCalibRow : TopCalibRow);
    printf("Epix100aConfigurator::writePixelBits calibration row 0x%x\n", blk.row());
    for (unsigned col=0; col<((1+BanksPerAsic)*PixelsPerBank); col++) {
      if (col == PixelsPerBank) col = halfRow; // do this for the first bank in each asic
      blk.col(col);
      //      printf("\tpixel %u calib %u\n", pixel&3, _config->calibPixelConfigArray()[row][col] & 3);
      //      if ( pixel&3 != (_config->calibPixelConfigArray()[row][col]&3)) {
      unsigned half = col < halfRow ? 0 : halfRow;
      for (unsigned i=0; i<BanksPerAsic; i++) {
        unsigned thisCol = half + (i * PixelsPerBank) + (col % PixelsPerBank);
        blk.b()[i] = _config->calibPixelConfigArray()(row,thisCol) & 3;
      }
      if (mySynch.take() == false) {
        printf("Epix100aConfigurator::writePixelBits synchronization failed on write of calib row %u\n", row);
        ret |= Failure;
      }
      if (_pgp->writeRegisterBlock(
          &_d,
          MultiplePixelWriteCommandAddr,
          (unsigned*)&blk,
          sizeof(block)/sizeof(uint32_t),
          Pds::Pgp::PgpRSBits::Waiting,
          (_debug&1)?true:false))
      {
        printf("Epix100aConfigurator::writePixelBits failed on calib row %u, col %u\n", row, col);
        ret |= Failure;
      }
      //      }
    }
  }
  if (mySynch.clear() == false) {
    printf("Epix100aConfigurator::writePixelBits synchronization failed to clear\n");
  }
  printf(" SynchBufferDepth:\n");
  for (unsigned i=0; i<writeAhead; i++) {
    if (synchDepthHisto[i] > 1) {
      printf("\t%3u - %5u\n", i, synchDepthHisto[i]);
    }
  }
  printf("Epix100aConfigurator::writePixelBits PrepareForReadout for ASIC:");
  for (unsigned index=0; index<_config->numberOfAsics(); index++) {
    if (m&(1<<index)) {
      uint32_t a = AsicAddrBase + AsicAddrOffset * index;
      if (_pgp->writeRegister( &_d, a+PrepareForReadout, 0, (_debug&1)?true:false, Pgp::PgpRSBits::Waiting)) {
        printf("Epix100aConfigurator::writePixelBits failed on ASIC %u\n", index);
        ret |= Failure;
      }
      printf(" %u", index);
      _getAnAnswer();
    }
  }
  printf("\n");
  if (_pgp->writeRegister( &_d, SaciClkBitAddr, SaciClkBitValue, (_debug&1)?true:false, Pgp::PgpRSBits::notWaiting)) {
    printf("Epix100aConfigurator::writePixelBits failed on writing SaciClkBitAddr\n");
    ret |= Failure;
  }
  return ret;
}


bool Epix100aConfigurator::_getAnAnswer(unsigned size, unsigned count) {
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

unsigned Epix100aConfigurator::checkWrittenASIC(bool writeBack) {
  unsigned ret = Success;
  bool done = false;
  while (!done && _config && _pgp) {
    printf("Epix100aConfigurator::checkWrittenASIC ");
    _d.dest(Epix100aDestination::Registers);
    unsigned m = _config->asicMask();
    uint32_t myBuffer[sizeof(Epix100aASIC_ConfigShadow)/sizeof(uint32_t)];
    Epix100aASIC_ConfigShadow* readAsic = (Epix100aASIC_ConfigShadow*) myBuffer;
    for (unsigned index=0; index<_config->numberOfAsics(); index++) {
      if (m&(1<<index)) {
        printf("reading ");
        uint32_t a = AsicAddrBase + (AsicAddrOffset * index);
        for (int i = 0; (i<Epix100aASIC_ConfigShadow::NumberOfValues) && (ret==Success); i++) {
          if ((AconfigAddrs[i][1] != WriteOnly) && _pgp) {
            if (_pgp->readRegister(&_d, a+AconfigAddrs[i][0], 0xa000+(index<<2)+i, myBuffer+i, 1)){
              printf("Epix100aConfigurator::checkWrittenASIC read reg %u failed on ASIC %u at 0x%x\n",
                  i, index, a+AconfigAddrs[i][0]);
              return Failure;
            }
            myBuffer[i] &= 0xffff;
          }
        }
        printf("checking ");
        Epix100aASIC_ConfigShadow* confAsic = (Epix100aASIC_ConfigShadow*) &( _config->asics(index));
        if ( (ret==Success) && (*confAsic != *readAsic) && _pgp) {
          printf("Epix100aConfigurator::checkWrittenASIC failed on ASIC %u\n", index);
          ret = Failure;
          if (writeBack) *confAsic = *readAsic;
        }
      }
    }
    printf("\n");
    done = true;
  }
  if (!_pgp) printf("Epix100aConfigurator::checkWrittenASIC found nil pgp\n");
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

void Epix100aConfigurator::dumpFrontEnd() {
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    pgp()->printStatus();
    printf("\tSequence Count(%u), Acquisition Count(%u)\n", sequenceCount(), acquisitionCount());
    printf("Environmental Data:\n");
    for (int i=0; i<NumberOfEnviroDatas; i++) {
      if (i<3) printf("\t%s%5.2f\n", enviroNames[i], 0.01*(signed int)enviroData(i));
      else printf("\t%s%5d\n", enviroNames[i], enviroData(i));
    }
  }
  if (_debug & 0x400) {
    //    printf("Checking Configuration, no news is good news ...\n");
    //    if (Failure == checkWrittenConfig(false)) {
    //      printf("Epix100aConfigurator::checkWrittenConfig() FAILED !!!\n");
    //    }
    //    enableRunTrigger(false);
    //    if (Failure == checkWrittenASIC(false)) {
    //      printf("Epix100aConfigurator::checkWrittenASICConfig() FAILED !!!\n");
    //    }
    //    enableRunTrigger(true);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("Epix100aConfigurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
}
