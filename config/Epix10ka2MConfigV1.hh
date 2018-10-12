/*
 * Epix10kaConfigV1.hh
 *
 *  Created on: 2017.10.31
 *      Author: jackp
 */

#ifndef EPIX10KA2MConfigV1_HH_
#define EPIX10KA2MConfigV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/Epix10kaASICConfigV1.hh"
#include <stdint.h>

namespace Pds {
  namespace Epix10ka2m {

    class ConfigV1 {
    public:
      ConfigV1(bool init=false);
      ConfigV1(const Epix::Config10ka2MV1&);
      ~ConfigV1();
      enum Epix10kaConstants {
        RowsPerAsic                = Epix::Config10ka::_numberOfRowsPerAsic,
        ColsPerAsic                = Epix::Config10ka::_numberOfPixelsPerAsicRow,
        ASICsPerRow                = Epix::Config10ka::_numberOfAsicsPerRow,
        ASICsPerCol                = Epix::Config10ka::_numberOfAsicsPerColumn,
        CalibrationRowCountPerAsic = Epix::Config10ka::_calibrationRowCountPerASIC
      };

      enum Registers {
        Version,
        UsePgpEvr,
        EvrRunCode,
        EvrDaqCode,
        EvrRunTrigDelay,
        EpixRunTrigDelay,

        //  Per Quad
        DcDcEn,
        AsicAnaEn,
        AsicDigEn,
        DdrVttEn,

        AcqCount,
        AcqToAsicR0Delay,
        AsicR0Width,
        AsicR0ToAsicAcq,
        AsicAcqWidth,
        AsicAcqLToPPmatL,
        AsicPPmatToReadout,
        AsicRoClkHalfT,
        AsicRoClkCount,
        AsicPreAcqTime,

        AdcPipelineDelay,

        ScopeTriggerParms1,
        ScopeTriggerParms2,
        ScopeWaveformSelects,
        ScopeTrigDelay,

        TestChannel,
        TestDataMask,
        TestPattern,
        TestSamples,
        TestTimeout,
        TestRequest,
        NumberOf_Registers};
      
      enum Ad_Registers {
        ChipId,
        DevIndexMask,
        DevIndexMaskDcoVco,
        ExtPwdnMode,
        IntPwdnMode,
        ChopMode,
        DutyCycleStab,
        OutputInvert,
        OutputFormat,
        ClockDivide,
        UserTestMode,
        OutputTestMode,
        OffsetAdjust,
        ChannelDelay,
        FrameDelay,
        NumberOf_Ad_Registers };

      enum Elem_Registers {
        carrierId0,
        carrierId1,
        NumberOf_Elem_Registers };

      enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2, DoNotUse=3 };
      enum types { decimal=0, hex=1, selection=2 };

      unsigned                    get      (Registers) const;
      void                        set      (Registers, unsigned);
      unsigned                    get      (Ad_Registers, unsigned) const;
      void                        set      (Ad_Registers, unsigned, unsigned);
      unsigned                    get      (Elem_Registers, unsigned) const;
      void                        set      (Elem_Registers, unsigned, unsigned);
      void                        clear();
      Pds::Epix10kaConfig::ASIC_ConfigV1*       asics()       { return (Pds::Epix10kaConfig::ASIC_ConfigV1*) (this+1); }
      const Pds::Epix10kaConfig::ASIC_ConfigV1* asics() const { return (const Pds::Epix10kaConfig::ASIC_ConfigV1*) (this+1); }
      uint32_t                    numberOfAsics() const;
      static uint32_t             offset    (Registers);
      static char*                name      (Registers);
      static void                 initNames();
      static unsigned             readOnly  (Registers);
      static unsigned             type      (Registers);
      static unsigned             numberOfSelections(Registers);
      static int                  select    (Registers, uint32_t);
      static int                  indexOfSelection(Registers, uint32_t);
      
    private:
      uint32_t     _values         [NumberOf_Registers];
      uint32_t     _ad_values  [10][NumberOf_Ad_Registers];
      uint32_t     _elem_values [4][NumberOf_Elem_Registers];
    };
  }
}

#endif
