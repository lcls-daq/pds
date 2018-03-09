/*
 * Epix10kaStatusRegisters.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/epix10ka/Epix10kaStatusRegisters.hh"

namespace Pds {
  namespace Epix10ka {

    void StatusLane::print() {
      printf("\t\tlocLink(%u) remLink(%u) pibLink(%u)\n",
          locLinkReady, remLinkReady, PibLinkReady);
      printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u)\n",
          cellErrorCount, linkErrorCount, linkDownCount);
      printf("\t\tbufferOverflowCount(%u) txCounter(%u)\n",
          bufferOverflowCount, txCounter);
    }

    void Epix10kaStatusRegisters::print() {
      printf("Epix10ka Status Registers: version(0x%x), scratchPad(0x%x)\n",
          version, scratchPad);
      for (unsigned i=0; i<NumberOfLanes; i++) {
        printf("\tLane %u:\n", i);
        lanes[i].print();
      }
    }
  }
}
