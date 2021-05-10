/*
 * Epix10kaASICConfigV1.cc
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#include <stdio.h>
#include "Epix10kaASICConfigV1.hh"

namespace Pds {
  namespace Epix10kaConfig {

    class Register {
      public:
        uint32_t offset;
        uint32_t shift;
        uint32_t mask;
        uint32_t defaultValue;
        ASIC_ConfigV1::readOnlyStates readOnly;
        ASIC_ConfigV1::copyStates doNotCopy;
    };

    static uint32_t _ASICfoo[][6] = {
   //offset  shift    mask   default readOnly doNotCopy
      { 0,     0,    0x3f,    0x1a,     0,    0}, //  CompTH_DAC
      { 0,     6,     0x1,       1,     0,    0}, //  CompEn_lowBit
      { 4,     6,     0x1,       1,     0,    0}, //  CompEn_midBit
      { 4,     7,     0x1,       1,     0,    0}, //  CompEn_topBit
      { 0,     7,     0x1,       0,     0,    0}, //  PulserSync
      { 1,     0,    0xff,    0x5a,     3,    0}, //  pixelDummy
      { 2,     0,   0x3ff,     0xA,     0,    0}, //  Pulser
      { 2,    10,     0x1,       0,     0,    0}, //  Pbit
      { 2,    11,     0x1,       0,     0,    0}, //  atest
      { 2,    12,     0x1,       0,     0,    0}, //  test
      { 2,    13,     0x1,       0,     0,    0}, //  Sab_test
      { 2,    14,     0x1,       0,     0,    0}, //  Hrtest
      { 2,    15,     0x1,       0,     0,    0}, //  PulserR
      { 3,     0,     0xf,       0,     0,    0}, //  DM1
      { 3,     4,     0xf,       1,     0,    0}, //  DM2
      { 4,     0,     0x7,       3,     0,    0}, //  Pulser_DAC
      { 4,     3,     0x7,       0,     0,    0}, //  Monost_Pulser
      { 5,     0,     0x1,       0,     0,    0}, //  DM1en
      { 5,     1,     0x1,       0,     0,    0}, //  DM2en
      { 5,     2,     0x7,       0,     0,    0}, //  emph_bd
      { 5,     5,     0x7,       0,     0,    0}, //  emph_bc
      { 6,     0,    0x3f,    0x13,     0,    0}, //  VREF_DAC
      { 6,     6,     0x3,       3,     0,    0}, //  VrefLow
      { 7,     0,     0x1,       1,     0,    0}, //  TPS_tcomp
      { 7,     1,     0xf,       0,     0,    0}, //  TPS_MUX
      { 7,     5,     0x7,       3,     0,    0}, //  RO_Monost
      { 8,     0,     0xf,       3,     0,    0}, //  TPS_GR
      { 8,     4,     0xf,       3,     0,    0}, //  S2D0_GR
      { 9,     0,     0x1,       1,     0,    0}, //  PP_OCB_S2D
      { 9,     1,     0x7,       3,     0,    0}, //  OCB
      { 9,     4,     0x7,       3,     0,    0}, //  Monost
      { 9,     7,     0x1,       0,     0,    0}, //  fastPP_enable
      { 10,    0,     0x7,       4,     0,    0}, //  Preamp
      { 10,    3,     0x7,       4,     0,    0}, //  PixelCB
      { 10,    6,     0x3,       1,     0,    0}, //  Vld1_b
      { 11,    0,     0x1,       0,     0,    0}, //  S2D_tcomp
      { 11,    1,    0x3f,    0x18,     0,    0}, //  Filter_DAC
      { 11,    7,     0x1,       0,     0,    0}, //  testLVDTransmitter
      { 12,    0,     0x3,       0,     0,    0}, //  tc
      { 12,    2,     0x7,       3,     0,    0}, //  S2D
      { 12,    5,     0x7,       3,     0,    0}, //  S2D_DAC_Bias
      { 13,    0,     0x3,       0,     0,    0}, //  TPS_tcDAC
      { 13,    2,    0x3f,    0x10,     0,    0}, //  TPS_DAC
      { 15,    0,     0x1,       0,     0,    0}, //  testBE
      { 15,    1,     0x1,       0,     0,    0}, //  is_en
      { 15,    2,     0x1,       0,     0,    0}, //  DelEXEC
      { 15,    3,     0x1,       0,     0,    0}, //  DelCCKreg
      { 15,    4,     0x1,       0,     0,    0}, //  RO_rst_en
      { 15,    5,     0x1,       1,     0,    0}, //  SLVDSbit
      { 15,    6,     0x1,       1,     0,    0}, //  FELmode
      { 15,    7,     0x1,       1,     0,    0}, //  CompEnOn
      { 16,    4,   0x1FF,       0,     3,    0}, //  RowStart
      { 17,    0,   0x1FF,    0xb1,     3,    0}, //  RowStop
      { 18,    0,    0x7F,       0,     3,    0}, //  ColumnStart
      { 19,    0,    0x7f,    0x2f,     3,    0}, //  ColumnStop
      { 20,    0,  0xffff,       0,     1,    0}, //  chipID
      { 21,    0,     0xf,       3,     0,    0}, //  S2D1_GR
      { 21,    4,     0xf,       3,     0,    0}, //  S2D2_GR
      { 22,    0,     0xf,       3,     0,    0}, //  S2D3_GR
      { 22,    4,     0x1,       1,     0,    0}, //  trbit
      { 14,    0,     0x3,       1,     0,    0}, //  S2D0_tcDAC
      { 14,    2,    0x3f,    0x14,     0,    0}, //  S2D0_DAC
      { 23,    0,     0x3,       1,     0,    0}, //  S2D1_tcDAC
      { 23,    2,    0x3f,    0x12,     0,    0}, //  S2D1_DAC
      { 24,    0,     0x3,       1,     0,    0}, //  S2D2_tcDAC
      { 24,    2,    0x3f,    0x12,     0,    0}, //  S2D2_DAC
      { 25,    0,     0x3,       1,     0,    0}, //  S2D3_tcDAC
      { 25,    2,    0x3f,    0x12,     0,    0}, //  S2D3_DAC
    };

    static char _ARegNames[ASIC_ConfigV1::NumberOfRegisters+1][120] = {
        "CompTH_DAC",
        "CompEn_lowB",
        "CompEn_midB",
        "CompEn_topB",
        "PulserSync",
        "pixelDummy",
        "Pulser",
        "Pbit",
        "atest",
        "test",
        "Sab_test",
        "Hrtest",
        "PulserR",
        "DM1",
        "DM2",
        "Pulser_daq",
        "MonostPulser",
        "DM1en",
        "DM2en",
        "emph_bd",
        "emph_bc",
        "VREF_DAC",
        "VrefLow",
        "TPS_tcomp",
        "TPS_MUX",
        "RO_Monost",
        "TPS_GR",
        "S2D0_GR",
        "PP_OCB_S2D",
        "OCB",
        "Monost",
        "fastPP_en",
        "Preamp",
        "Pixel_CB",
        "Vldl_b",
        "S2D_tcomp",
        "Filter_DAC",
        "testLVDTx",
        "tc",
        "S2D",
        "S2D_DAC_Bias",
        "TPS_tcDAC",
        "TPS_DAC",
        "testBE",
        "is_en",
        "DelEXEC",
        "DelCCKreg",
        "RO_rst_en",
        "SLVDSbit",
        "FELmode",
        "CompEnOn",
        "RowStart",
        "RowStop",
        "ColumnStart",
        "ColumnStop",
        "chipID",
        "S2D1_GR",
        "S2D2_GR",
        "S2D3_GR",
        "trbit",
        "S2D0_tcDAC",
        "S2D0_DAC",
        "S2D1_tcDAC",
        "S2D1_DAC",
        "S2D2_tcDAC",
        "S2D2_DAC",
        "S2D3_tcDAC",
        "S2D3_DAC",
        {"--INVALID--"}        //    NumberOfRegisters
    };


    static Register* _Aregs = (Register*) _ASICfoo;
    static bool      namesAreInitialized = false;

    ASIC_ConfigV1::ASIC_ConfigV1(bool init) {
      if (init) {
        for (int i=0; i<NumberOfValues; i++) {
          _values[i] = 0;
        }
      }
    }

    ASIC_ConfigV1::~ASIC_ConfigV1() {}

    unsigned   ASIC_ConfigV1::offset(ASIC_ConfigV1::Registers r) const {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::index parameter out of range!! %u\n", r);
        return -1;
      }
      return _Aregs[r].offset;
    }

    unsigned   ASIC_ConfigV1::mask(ASIC_ConfigV1::Registers r) const {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::index parameter out of range!! %u\n", r);
        return -1;
      }
      return _Aregs[r].mask << _Aregs[r].shift;
    }

    unsigned   ASIC_ConfigV1::get (ASIC_ConfigV1::Registers r) const {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::get parameter out of range!! %u\n", r);
        return 0;
      }
      return ((_values[_Aregs[r].offset] >> _Aregs[r].shift) & _Aregs[r].mask);
    }

    void   ASIC_ConfigV1::set (ASIC_ConfigV1::Registers r, unsigned v) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::set parameter out of range!! %u\n", r);
        return;
      }
      _values[_Aregs[r].offset] &= ~(_Aregs[r].mask << _Aregs[r].shift);
      _values[_Aregs[r].offset] |= (_Aregs[r].mask & v) << _Aregs[r].shift;
      return;
    }

    void   ASIC_ConfigV1::clear() {
      memset(_values, 0, NumberOfValues*sizeof(uint32_t));
    }

    uint32_t   ASIC_ConfigV1::rangeHigh(ASIC_ConfigV1::Registers r) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::rangeHigh parameter out of range!! %u\n", r);
        return 0;
      }
      return _Aregs[r].mask;
    }

    uint32_t   ASIC_ConfigV1::rangeLow(ASIC_ConfigV1::Registers) {
      return 0;
    }

    uint32_t   ASIC_ConfigV1::defaultValue(ASIC_ConfigV1::Registers r) {
      if (r >= ASIC_ConfigV1::NumberOfRegisters) {
        printf("ASIC_ConfigV1::defaultValue parameter out of range!! %u\n", r);
        return 0;
      }
      return _Aregs[r].defaultValue & _Aregs[r].mask;
    }

    char* ASIC_ConfigV1::name(ASIC_ConfigV1::Registers r) {
      return r < ASIC_ConfigV1::NumberOfRegisters ? _ARegNames[r] : _ARegNames[ASIC_ConfigV1::NumberOfRegisters];
    }

    void ASIC_ConfigV1::initNames() {
      static char range[60];
      if (namesAreInitialized == false) {
        for (unsigned i=0; i<NumberOfRegisters; i++) {
          Registers r = (Registers) i;
          sprintf(range, "  (0x%x->0x%x)    ", rangeLow(r), rangeHigh(r));
          if (_Aregs[r].readOnly != ReadOnly) {
            strncat(_ARegNames[r], range, 40);
//            printf("ASIC_ConfigV1::initNames %u %s %s\n", i, range, _ARegNames[r]);
          }
        }
      }
      namesAreInitialized = true;
    }

    unsigned ASIC_ConfigV1::readOnly(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::Epix10kaSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return UseOnly;
      }
      return (unsigned)_Aregs[r].readOnly;
    }

    unsigned ASIC_ConfigV1::doNotCopy(Registers r) {
      if (r >= NumberOfRegisters) {
        printf("Pds::Epix10kaSamplerConfig::ConfigV1::readOnly parameter out of range!! %u\n", r);
        return DoNotCopy;
      }
      return (unsigned)_Aregs[r].doNotCopy;
    }

    void ASIC_ConfigV1::operator=(ASIC_ConfigV1& foo) {
      unsigned i=0;
      while (i<NumberOfRegisters) {
        Registers c = (Registers)i++;
        if ((readOnly(c) == ReadWrite) && (doNotCopy(c) == DoCopy)) set(c,foo.get(c));
      }
    }

    bool ASIC_ConfigV1::operator==(ASIC_ConfigV1& foo) {
      unsigned i=0;
      bool ret = true;
      while (i<NumberOfRegisters/* && ret*/) {
        Registers c = (Registers)i++;
        if (doNotCopy(c) == DoCopy) {
          // don't compare if not ReadWrite
          bool lret = ( (readOnly(c) != ReadWrite) || (get(c) == foo.get(c)));
          if (lret == false) {
            printf("\tEpix10ka ASIC_ConfigV1 %u != %u at %s\n", get(c), foo.get(c), ASIC_ConfigV1::name(c));
          }
          ret = ret && lret;
        }
      }
      return ret;
    }

  } /* namespace Epix10kaConfig */
} /* namespace Pds */
