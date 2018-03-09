/*
 * Epix10kaConfigV1.hh
 *
 *  Created on: 2017.10.31
 *      Author: jackp
 */

#ifndef EPIX10KAConfigV1_HH_
#define EPIX10KAConfigV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/Epix10kaASICConfigV1.hh"
#include <stdint.h>



namespace Pds {
  namespace Epix10kaConfig {

    class ASIC_ConfigV1;

    class ConfigV1 {
      public:
        ConfigV1(bool init=false);
        ~ConfigV1();
        enum Epix10kaConstants {
          RowsPerAsic = 176,
          ColsPerAsic = 192,
          ASICsPerRow = 2,
          ASICsPerCol = 2,
          CalibrationRowCountPerAsic = 2
        };

        enum Registers {
          Version,
          UsePgpEvr,
          EvrRunCode,
          EvrDaqCode,
          EvrRunTrigDelay,
          EpixRunTrigDelay,
          DacSetting,
          asicGR,
          asicGRControl,
          asicAcq,
          asicAcqControl,
          asicR0,
          asicR0Control,
          asicPpmat,
          asicPpmatControl,
          asicPpbe,
          asicPpbeControl,
          asicRoClk,
          asicRoClkControl,
          prepulseR0En,
          adcStreamMode,
          testPatternEnable,
          SyncMode,
          R0Mode,
          AcqToAsicR0Delay,
          AsicR0ToAsicAcq,
          AsicAcqWidth,
          AsicAcqLToPPmatL,
          AsicPPmatToReadout,
          AsicRoClkHalfT,
          AdcReadsPerPixel,
          AdcClkHalfT,
          AsicR0Width,
          AdcPipelineDelay,
          AdcPipelineDelay0,
          AdcPipelineDelay1,
          AdcPipelineDelay2,
          AdcPipelineDelay3,
          SyncWidth,
          SyncDelay,
          PrepulseR0Width,
          PrepulseR0Delay,
          DigitalCardId0,
          DigitalCardId1,
          AnalogCardId0,
          AnalogCardId1,
          CarrierId0,
          CarrierId1,
          NumberOfAsicsPerRow,
          NumberOfAsicsPerColumn,
          NumberOfRowsPerAsic,
          NumberOfReadableRowsPerAsic,
          NumberOfPixelsPerAsicRow,
          CalibrationRowCountPerASIC,
          EnvironmentalRowCountPerASIC,
          BaseClockFrequency,
          AsicMask,
          EnableAutomaticRunTrigger,
          NumbClockTicksPerRunTrigger,
          ScopeEnable,
          ScopeTrigEdge,
          ScopeTrigCh,
          ScopeArmMode,
          ScopeAdcThresh,
          ScopeHoldoff,
          ScopeOffset,
          ScopeTraceLength,
          ScopeSkipSamples,
          ScopeInputA,
          ScopeInputB,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2, DoNotUse=3 };
        enum types { decimal=0, hex=1, selection=2 };

        enum {
          NumberOfValues=47,
          NumberOfSelects=1
        };

        enum {
          NumberOfAsics=4, NumberOfASICs=4    //  Kludge ALERT!!!!!  this may need to be dynamic !!
        };

        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        void                        clear();
        ASIC_ConfigV1*              asics() { return (ASIC_ConfigV1*) (this+1); }
        const ASIC_ConfigV1*        asics() const { return (const ASIC_ConfigV1*) (this+1); }
        uint32_t                    numberOfAsics() const;
        static uint32_t             offset    (Registers);
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        static unsigned             type(Registers);
        static unsigned             numberOfSelections(Registers);
        static int                  select(Registers, uint32_t);
        static int                  indexOfSelection(Registers, uint32_t);

      private:
        uint32_t     _values[NumberOfValues];
//        ASIC_ConfigV1 _asics[NumberOfAsics];
//        uint32_t     _pixelTestArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
//        uint32_t     _pixelMaskArray[get(NumberOfAsicsPerRow)*get(NumberOfAsicsPerColumn)][get(NumberOfRowsPerAsic)][(get(NumberOfPixelsPerAsicRow)+31)/32];
    };

  } /* namespace ConfigV1 */
} /* namespace Pds */

#endif /* EPIX10KAConfigV1_HH_ */
