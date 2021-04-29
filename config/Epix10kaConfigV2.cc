/*
 * Epix10kaConfigV2.cc
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/config/Epix10kaConfigV2.hh"
#include "pds/config/Epix10kaASICConfigV1.hh"
#include "pds/config/EpixConfigType.hh"

namespace Pds {
  namespace Epix10kaConfig {

    class  StrRegister {
      public:
        uint32_t        offset;
        uint32_t        maxchars;
        uint32_t        size;
        uint32_t        readOnly;
        uint32_t        ishash;
    };

    class  Register {
      public:
        uint32_t        offset;
        uint32_t        shift;
        uint32_t        mask;
        uint32_t        defaultValue;
        uint32_t        readOnly;
        ConfigV2::types type;
        uint32_t        selectionIndex;
    } ;
//    asicPpmatControl = 0x0
//    AsicRoClkHalfT = 0xA

    static uint32_t _regsstr[ConfigV2::NumberOfStrRegisters][5] = {
    //offset maxchars size readOnly
        { 0,   64,  5, 1, 1},   //  FirmwareHash
        { 64, 256, 64, 1, 0},   //  FirmwareDesc
    };

    static uint32_t _regsfoo[ConfigV2::NumberOfRegisters][7] = {
    //offset shift mask default readOnly type selectionIndex
        {  0,  0, 0xffffffff, 0,      1, 1, 0},    //  Version,
        {  1,  0, 1,          0,      3, 0, 0},    //  UsePgpEvr,
        {  2,  0, 0xff,      40,      2, 0, 0},    //  evrRunCode,
        {  3,  0, 0xff,      40,      2, 0, 0},    //  evrDaqCode,
        {  4,  0, 0x7fffffff, 1,      2, 0, 0},    //  evrRunTrigDelay,
        {  5,  0, 0x7fffffff, 1,      0, 0, 0},    //  epixRunTrigDelay,
        {  6,  0, 0x7fffffff, 1,      0, 0, 0},    //  epixDaqTrigDelay,
        {  7,  0, 0xffff,     0,      0, 1, 0},    //  DacSetting,
        //  pin states(4) and controls(5)
        {  8,  0, 1, 0, 0, 0, 0},    //  asicGR,
        {  9,  0, 1, 0, 0, 0, 0},    //  asicGRControl,
        {  8,  1, 1, 0, 0, 0, 0},    //  asicAcq,
        {  9,  1, 1, 0, 0, 0, 0},    //  asicAcqControl,
        {  8,  2, 1, 0, 0, 0, 0},    //  asicR0,
        {  9,  2, 1, 0, 0, 0, 0},    //  asicR0Control,
        {  8,  3, 1, 1, 0, 0, 0},    //  asicPpmat,
        {  9,  3, 1, 1, 0, 0, 0},    //  asicPpmatControl,
        {  8,  4, 1, 0, 0, 0, 0},    //  asicPpbe,
        {  9,  4, 1, 0, 0, 0, 0},    //  asicPpbeControl,
        {  8,  5, 1, 0, 0, 0, 0},    //  asicRoClk,
        {  9,  5, 1, 0, 0, 0, 0},    //  asicR0ClkControl,
        {  9,  6, 1, 1, 3, 0, 0},    //  prepulseR0En,
        {  9,  7, 1, 0, 0, 0, 0},    //  adcStreamMode,
        {  9,  8, 1, 0, 0, 0, 0},    //  testPatternEnable,
        //
        { 10,  0, 0x7fffffff,14032,      0, 0, 0},    //  AcqToAsicR0Delay,
        { 11,  0, 0x7fffffff, 5000,      0, 0, 0},    //  AsicR0ToAsicAcq,
        { 12,  0, 0x7fffffff, 5000,      0, 0, 0},    //  AsicAcqWidth,
        { 13,  0, 0x7fffffff,  200,      0, 0, 0},    //  AsicAcqLToPPmatL,
        { 14,  0, 0x7fffffff,    0,      0, 0, 0},    //  AsicPPmatToReadout,
        { 15,  0, 0x7fffffff,    5,       0, 0, 0},    //  AsicRoClkHalfT,
        { 16,  0, 0x7fffffff,    1,       0, 0, 0},    //  AdcClkHalfT,
        { 17,  0, 0x7fffffff,   30,       0, 0, 0},    //  AsicR0Width,
        { 18,  0, 0x7fffffff,   32,       3, 0, 0},    //  AdcPipelineDelay,
        { 19,  0, 0x7fffffff,   32,       0, 0, 0},    //  AdcPipelineDelay0,
        { 20,  0, 0x7fffffff,   32,       0, 0, 0},    //  AdcPipelineDelay1,
        { 21,  0, 0x7fffffff,   32,       0, 0, 0},    //  AdcPipelineDelay2,
        { 22,  0, 0x7fffffff,   32,       0, 0, 0},    //  AdcPipelineDelay3,
        { 23,  0, 0xffff,       30,       3, 0, 0},    //  SnycWidth,
        { 23, 16, 0xffff,       30,       3, 0, 0},    //  SyncDelay,
        { 24,  0, 0x7fffffff,   30,       3, 0, 0},    //  PrepulseR0Width,
        { 25,  0, 0x7fffffff, 32000,      3, 0, 0},    //  PrepulseR0Delay,
        { 26,  0, 0x7fffffff, 0,          1, 0, 0},    //  DigitalCardId0,
        { 27,  0, 0x7fffffff, 0,          1, 0, 0},    //  DigitalCardId1,
        { 28,  0, 0x7fffffff, 0,          1, 0, 0},    //  AnalogCardId0,
        { 29,  0, 0x7fffffff, 0,          1, 0, 0},    //  AnalogCardId1,
        { 30,  0, 0x7fffffff, 0,          1, 0, 0},    //  CarrierId0,
        { 31,  0, 0x7fffffff, 0,          1, 0, 0},    //  CarrierId1,
        { 32,  0, 0xf,        2,          2, 0, 0},    //  NumberOfAsicsPerRow,
        { 33,  0, 0xf,        2,          2, 0, 0},    //  NumberOfAsicsPerColumn,
        { 34,  0, 0x1ff,      ConfigV2::RowsPerAsic,2, 0, 0},    //  NumberOfRowsPerAsic,
        { 35,  0, 0x1ff,      ConfigV2::RowsPerAsic,2, 0, 0},    //  NumberOfReadableRowsPerAsic,
        { 36,  0, 0x1ff,      ConfigV2::ColsPerAsic,2, 0, 0},    //  NumberOfPixelsPerAsicRow,
        { 37,  0, 0x1ff,      2,          2, 0, 0},    //  CalibrationRowCountPerASIC,
        { 38,  0, 0x1ff,      1,          2, 0, 0},    //  EnvironmentalRowCountPerASIC,
        { 39,  0, 0x7fffffff, 0,          1, 0, 0},    //  BaseClockFrequency,
        { 40,  0, 0xffff,     0xf,        0, 1, 0},    //  AsicMask,
        { 41,  0, 1,          0,          0, 0, 0},    //  EnableAutomaticRunTrigger,
        { 42,  0, 0x7fffffff, 1080729,    0, 2, 0},    //  NumbClockTicksPerRunTrigger,
        { 43,  0, 1,          1,          0, 0, 0},    //  GhostCorrEn,
        { 44,  0, 1,          0,          0, 0, 0},    //  OversampleEn,
        { 45,  0, 0x7,        0,          0, 0, 0},    //  OversampleSize,
        { 46,  0, 1,          0,          0, 0, 0},    //  ScopeEnable,
        { 46,  1, 1,          1,          0, 0, 0},    //  ScopeTrigEdge,
        { 46,  2, 0xf,        6,          0, 0, 0},    //  ScopeTrigCh,
        { 46,  6, 3,          2,          0, 0, 0},    //  ScopeArmMode,
        { 46, 16, 0xffff,     0,          0, 1, 0},    //  ScopeAdcThresh,
        { 47,  0, 0x1fff,     0,          0, 0, 0},    //  ScopeHoldoff,
        { 47, 13, 0x1fff,     0xf,        0, 0, 0},    //  ScopeOffset,
        { 48,  0, 0x1fff,     0x1000,     0, 1, 0},    //  ScopeTraceLength,
        { 48, 13, 0x1fff,     0,          0, 0, 0},    //  ScopeSkipSamples,
        { 49,  0, 0x1f,       0,          0, 0, 0},    //  ScopeInputA,
        { 49,  5, 0x1f,       4,          0, 0, 0},    //  ScopeInputB,
    };

    static uint32_t _regSelect0[7] = {
    		6,
    		833334,
    		1666667,
    		3333334,
    		10000000,
    		20000000,
    		100000000
    };

    static uint32_t* _regSelects[ConfigV2::NumberOfSelects] = {
    		_regSelect0
    };

    static char _strRegNames[ConfigV2::NumberOfStrRegisters+1][120] = {
        {"FirmwareHash"},
        {"FirmwareDesc"},
        {"-------INVALID------------"}//NumberOfStrRegisters
    };

    static char _regNames[ConfigV2::NumberOfRegisters+1][120] = {
        {"Version"},
        {"UsePgpEvr"},
        {"EvrRunCode"},
        {"EvrDaqCode"},
        {"EvrRunTrigDelay"},
        {"EpixRunTrigDelay"},
        {"EpixDaqTrigDelay"},
        {"DacSetting"},
        {"asicGR"},
        {"asicGRControl"},
        {"asicAcq"},
        {"asicAcqControl"},
        {"asicR0"},
        {"asicR0Control"},
        {"asicPpmat"},
        {"asicPpmatControl"},
        {"asicPpbe"},
        {"asicPpbeControl"},
        {"asicRoClk"},
        {"asicRoClkControl"},
        {"prepulseR0En"},
        {"adcStreamMode"},
        {"testPatternEnable"},
        {"AcqToAsicR0Delay"},
        {"AsicR0ToAsicAcq"},
        {"AsicAcqWidth"},
        {"AsicAcqLToPPmatL"},
        {"AsicPPmatToReadout"},
        {"AsicRoClkHalfT"},
        {"AdcClkHalfT"},
        {"AsicR0Width"},
        {"AdcPipelineDelay"},
        {"AdcPipelineDelay0"},
        {"AdcPipelineDelay1"},
        {"AdcPipelineDelay2"},
        {"AdcPipelineDelay3"},
        {"SyncWidth"},
        {"SyncDelay"},
        {"PrepulseR0Width"},
        {"PrepulseR0Delay"},
        {"DigitalCardId0"},
        {"DigitalCardId1"},
        {"AnalogCardId0"},
        {"AnalogCardId1"},
        {"CarrierId0"},
        {"CarrierId1"},
        {"NumberOfAsicsPerRow"},
        {"NumberOfAsicsPerColumn"},
        {"NumberOfRowsPerAsic"},
        {"NumberOfReadableRowsPerAsic"},
        {"NumberOfPixelsPerAsicRow"},
        {"CalibrationRowCountPerASIC"},
        {"EnvironmentalRowCountPerASIC"},
        {"BaseClockFrequency"},
        {"AsicMask"},
        {"EnableAutomaticRunTrigger"},
        {"NumbClockTicksPerRunTrigger"},
        {"GhostCorrEn"},
        {"OversampleEn"},
        {"OversampleSize"},
        {"ScopeEnable"},
        {"ScopeTrigEdge"},
        {"ScopeTrigCh"},
        {"ScopeArmMode"},
        {"ScopeAdcThresh"},
        {"ScopeHoldoff"},
        {"ScopeOffset"},
        {"ScopeTraceLength"},
        {"ScopeSkipSamples"},
        {"ScopeInputA"},
        {"ScopeInputB"},
        {"-------INVALID------------"}//NumberOfRegisters
    };

    static Register* _regs = (Register*) _regsfoo;
    static StrRegister* _strRegs = (StrRegister*) _regsstr;
    static bool namesAreInitialized = false;

    ConfigV2::ConfigV2(bool init) {
      if (init) 
        clear();
    }

    ConfigV2::~ConfigV2() {}

    unsigned   ConfigV2::get (Registers r) const {
      if (r >= NumberOfRegisters) {
        printf("ConfigV2::get parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return 0;
      }
      return ((_values[_regs[r].offset] >> _regs[r].shift) & _regs[r].mask);
    }

    void   ConfigV2::set (Registers r, unsigned v) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV2::set parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return;
      }
      _values[_regs[r].offset] &= ~(_regs[r].mask << _regs[r].shift);
      _values[_regs[r].offset] |= (_regs[r].mask & v) << _regs[r].shift;
      return;
    }

    const char* ConfigV2::getStr  (StrRegisters r) const {
      if (r >= NumberOfStrRegisters) {
        printf("ConfigV2::getStr parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return NULL;
      }
      return &_strValues[_strRegs[r].offset];
    }

    void        ConfigV2::setStr  (StrRegisters r, const char* v) {
      if (r >= NumberOfStrRegisters) {
        printf("ConfigV2::setStr parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return;
      }

      if (v && (strlen(v) < _strRegs[r].maxchars)) {
        ::strcpy(&_strValues[_strRegs[r].offset], v);
      } else if(v) {
        printf("ConfigV2::setStr value length too long!! %lu > %u\n", strlen(v), _strRegs[r].maxchars);
      } else {
        printf("ConfigV2::setStr passed value is null!!\n");
      }
      return;
    }

    void   ConfigV2::clear() {
      memset(_values, 0, NumberOfValues*sizeof(uint32_t));
      memset(_strValues, 0, NumberOfChars);
      for(unsigned i=0; i<(defaultValue(NumberOfAsicsPerRow)*defaultValue(NumberOfAsicsPerColumn)); i++)
	      asics()[i].clear();
    }

    uint32_t ConfigV2::offset(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV2::set parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return 0xffffffff;
      }
      return _regs[r].offset;
    }

    uint32_t ConfigV2::numberOfAsics() const {
      return get(NumberOfAsicsPerRow) * get(NumberOfAsicsPerColumn);
    }

    uint32_t   ConfigV2::rangeHigh(Registers r) {
      uint32_t ret = _regs[r].mask;
      if (r >= NumberOfRegisters) {
        printf("ConfigV2::rangeHigh parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return 0;
      }
      switch (r) {
        case AdcClkHalfT :
          ret = 400;
          break;
        case NumberOfRowsPerAsic :
        case NumberOfReadableRowsPerAsic :
          ret = RowsPerAsic;
          break;
        case NumberOfPixelsPerAsicRow :
          ret = ColsPerAsic;
          break;
        case NumberOfAsicsPerRow :
          ret = ConfigV2::ASICsPerRow;
          break;
        case NumberOfAsicsPerColumn :
          ret = ConfigV2::ASICsPerCol;
          break;
        case CalibrationRowCountPerASIC :
          ret = 2;
          break;
        case EnvironmentalRowCountPerASIC :
          ret = 1;
          break;
        case AsicMask :
          ret = 15;
          break;
        default:
          break;
      }
      return ret;
    }

    uint32_t   ConfigV2::rangeLow(Registers r) {
      uint32_t ret = 0;
      switch (r) {
        case NumberOfRowsPerAsic :
          ret = RowsPerAsic;
          break;
        case NumberOfPixelsPerAsicRow :
          ret = ColsPerAsic;
          break;
        case AdcClkHalfT :
          ret = 1;
          break;
        case NumberOfAsicsPerRow :
          ret = ConfigV2::ASICsPerRow;
          break;
        case NumberOfAsicsPerColumn :
          ret = ConfigV2::ASICsPerCol;
          break;
        case CalibrationRowCountPerASIC :
          ret = 2;
          break;
        case EnvironmentalRowCountPerASIC :
          ret = 1;
          break;
        default:
          break;
      }
      return ret;
    }

    uint32_t   ConfigV2::defaultValue(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("ConfigV2::defaultValue parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return 0;
      }
      return _regs[r].defaultValue & _regs[r].mask;
    }

    char* ConfigV2::name(Registers r) {
      return r < NumberOfRegisters ? _regNames[r] : _regNames[NumberOfRegisters];
    }

    void ConfigV2::initNames() {
      static char range[60];
      if (namesAreInitialized == false) {
        for (unsigned i=0; i<NumberOfRegisters; i++) {
          Registers r = (Registers) i;
          types t = (types) type(r);
          if (t==hex) sprintf(range, "  (0x%x->0x%x)    ", rangeLow(r), rangeHigh(r));
          else if (t==decimal) sprintf(range, "  (%u->%u)    ", rangeLow(r), rangeHigh(r));
          if ((_regs[r].readOnly != ReadOnly) && (_regs[r].readOnly != DoNotUse) && (t != selection)) {
            strncat(_regNames[r], range, 40);
//            printf("ConfigV2::initNames %u %s %s\n", i, range, _regNames[r]);
          }
        }
        ASIC_ConfigV1::initNames();
      } else {
//        printf("ASIC_ConfigV1::initNames namesAreInitialized=%s\n", namesAreInitialized ? "true" : "false");
      }
      namesAreInitialized = true;
    }

    unsigned ConfigV2::readOnly(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Epix10kaConfigV2::readOnly parameter out of range!! %u %u\n", r, NumberOfRegisters);
        return 400;
      }
      return (unsigned)_regs[r].readOnly;
    }

    unsigned ConfigV2::type(Registers r) {
        if (r >= NumberOfRegisters) {
          printf("Epix10kaConfigV2::type parameter out of range!! %u, numregs %u\n", r, NumberOfRegisters);
          return 400;
        }
        return (unsigned)_regs[r].type;
    }

    unsigned ConfigV2::numberOfSelections(Registers r) {
        if (r >= NumberOfRegisters) {
          printf("Epix10kaConfigV2::numberOfselections parameter out of range!! %u %u\n", r, NumberOfRegisters);
          return 0;
        }
        if ((_regs[r].type == ConfigV2::selection) && (_regs[r].selectionIndex < NumberOfSelects)) {
        	return (unsigned)(_regSelects[_regs[r].selectionIndex][0]);
        }
        return 0;
    }

    int ConfigV2::select(Registers r, uint32_t s) {
    	unsigned n = ConfigV2::numberOfSelections(r);
    	if (n) {
    		return (int) _regSelects[_regs[r].selectionIndex][s+1];
    	}
    	return -1;
    }

    int ConfigV2::indexOfSelection(Registers r, uint32_t s) {
        unsigned i = 0;
    	unsigned n = ConfigV2::numberOfSelections(r);
    	if (n) {
    		do {
    			if (s == _regSelects[_regs[r].selectionIndex][i+1]) return i;
    		} while (i++ < n);
    	}
    	return -1;
    }

    uint32_t ConfigV2::offset(StrRegisters r) {
      if (r >= NumberOfStrRegisters) {
        printf("ConfigV2::offset parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return 0xffffffff;
      }
      return _strRegs[r].offset;
    }

    char* ConfigV2::name(StrRegisters r) {
      return r < NumberOfStrRegisters ? _strRegNames[r] : _strRegNames[NumberOfStrRegisters];
    }

    uint32_t ConfigV2::maxchars(StrRegisters r) {
      if (r >= NumberOfStrRegisters) {
        printf("ConfigV2::maxchars parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return 0;
      }
      return _strRegs[r].maxchars;
    }

    uint32_t ConfigV2::size(StrRegisters r) {
      if (r >= NumberOfStrRegisters) {
        printf("ConfigV2::size parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return 0;
      }
      return _strRegs[r].size;
    }

    unsigned ConfigV2::readOnly(StrRegisters r) {
      if (r >= NumberOfStrRegisters) {
        printf("Epix10kaConfigV2::readOnly parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return 400;
      }
      return (unsigned)_strRegs[r].readOnly;
    }

    unsigned ConfigV2::ishash(StrRegisters r) {
      if (r >= NumberOfStrRegisters) {
        printf("Epix10kaConfigV2::ishash parameter out of range!! %u %u\n", r, NumberOfStrRegisters);
        return 400;
      }
      return (unsigned)_strRegs[r].ishash;
    } 

  } /* namespace Epix10kaConfig */
} /* namespace Pds */
