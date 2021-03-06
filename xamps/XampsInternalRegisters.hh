/*
 * XampsInternalRegisters.hh
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#ifndef XAMPSINTERNALREGISTERS_HH_
#define XAMPSINTERNALREGISTERS_HH_

//#include <stdint.h>

namespace Pds {

  namespace Xamps {

    class InternalLane {
      public:
        InternalLane() {};
        ~InternalLane() {};
        void print();
      public:
        unsigned locLinkReady:          1;  //0
        unsigned remLinkReady:          1;  //1
        unsigned linked:                1;  //2
        unsigned unused1:               1;  //3
        unsigned cellErrorCount:        4;  //7:4
        unsigned unused2:               4;  //11:8
        unsigned linkErrorCount:        4;  //15:12
        unsigned linkDownCount:         4;  //19:16
        unsigned unused3:               4;  //23:20
        unsigned bufferOverflowCount:   4;  //27:24
        unsigned unused4:               4;  //28:31
        unsigned txCounter;
    };

    class XampsInternalRegisters {
      public:
        XampsInternalRegisters() {};
        ~XampsInternalRegisters() {};

        void     print();
        enum {NumberOfLanes=4};

      public:
        unsigned        version;
        unsigned        scratchPad;
        InternalLane    lanes[NumberOfLanes];
    };

  }

}

#endif /* XAMPSINTERNALREGISTERS_HH_ */
