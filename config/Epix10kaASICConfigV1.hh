/*
 * Epix10kaASICConfigV1.hh
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#ifndef EPIX10KAASICCONFIGV1_HH_
#define EPIX10KAASICCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include <stdint.h>
#include <string.h>

namespace Pds {
  namespace Epix10kaConfig {

    class ASIC_ConfigV1 {
      public:
        ASIC_ConfigV1(bool init=false);
        ~ASIC_ConfigV1();

        enum Registers {
          CompTH_DAC,
          CompEn_lowBit,
          CompEn_midBit,
          CompEn_topBit,
          PulserSync,
          pixelDummy,
          Pulser,
          Pbit,
          atest,
          test,
          Sab_test,
          Hrtest,
          PulserR,
          DM1,
          DM2,
          Pulser_daq,
          Monost_Pulser,
          DM1en,
          DM2en,
          emph_bd,
          emph_bc,
          VREF_DAC,
          VrefLow,
          TPS_tcomp,
          TPS_MUX,
          RO_Monost,
          TPS_GR,
          S2D0_GR,
          PP_OCB_S2D,
          OCB,
          Monost,
          fastPP_enable,
          Preamp,
          Pixel_CB,
          Vldl_b,
          S2D_tcomp,
          Filter_DAC,
          testLVDTransmitter,
          tc,
          S2D,
          S2D_DAC_Bias,
          TPS_tcDAC,
          TPS_DAC,
          testBE,
          is_en,
          DelEXEC,
          DelCCKreg,
          RO_rst_en,
          SLVDSbit,
          FELmode,
          CompEnOn,
          RowStart,
          RowStop,
          ColumnStart,
          ColumnStop,
          chipID,
          S2D1_GR,
          S2D2_GR,
          S2D3_GR,
          trbit,
          S2D0_tcDAC,
          S2D0_DAC,
          S2D1_tcDAC,
          S2D1_DAC,
          S2D2_tcDAC,
          S2D2_DAC,
          S2D3_tcDAC,
          S2D3_DAC,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };
        enum copyStates { DoCopy=0, DoNotCopy=1 };

        enum {
          NumberOfValues=26
        };

        unsigned                    offset   (Registers) const;
        unsigned                    mask     (Registers) const;
        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        void                        clear    ();
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        static unsigned             doNotCopy(Registers);
        void                        operator=(ASIC_ConfigV1&);
        bool                        operator==(ASIC_ConfigV1&);
        bool                        operator!=(ASIC_ConfigV1& foo) { return !(*this==foo); }

      private:
        uint32_t              _values[NumberOfValues];
    };

  } /* namespace Epix10kaConfig */
} /* namespace Pds */

#endif /* EPIX100AASICCONFIGV1_HH_ */
