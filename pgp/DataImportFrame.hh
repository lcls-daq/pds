/*
 * DataImportFrame.hh
 *
 *  Created on: Jun 8, 2010
 *      Author: jackp
 */

#ifndef DATAIMPORTFRAME_HH_
#define DATAIMPORTFRAME_HH_

//   Header = 8 x 32-bits
//       Header[0] = Tid[23:0], Lane[1:0], Z[3:0], VC[1:0]
//       Header[1] = Z[3:0], Quad[3:0], OpCode[7:0], acqCount[15:0]
//       Header[2] = BeamCode[31:0]
//       Header[3] = Z[31:0]
//       Header[4] = Z[31:0]
//       Header[5] = Z[31:0]
//       Header[6] = DiodeCurrent[31:0] (inserted by software)
//       Header[7] = FrameType[31:0]

namespace Pds {
  namespace Pgp {

    class FirstWordBits {
      public:
        unsigned vc:       2;    // 1:0
        unsigned z:        4;    // 5:2
        unsigned lane:     3;    // 8:6
        unsigned tid:     23;    //31:9
    };

    class SecondWordBits {
      public:
        unsigned acqCount:16;    //15:0
        unsigned opCode:   8;    //23:16
        unsigned elementID:4;    //27:24
        unsigned z:        4;    //31:28
    };

    class DataImportFrame {
      public:
        DataImportFrame() {};
        ~DataImportFrame() {};

      public:
        unsigned vc()          const { return first.vc; }
        unsigned lane()        const { return first.lane; }
        unsigned elementId()   const { return second.elementID; }
        unsigned acqCount()    const { return second.acqCount; }
        unsigned opCode()      const { return second.opCode; }
        unsigned frameNumber() const { return (unsigned) _frameNumber; }
        uint32_t ticks()       const { return _ticks; }
        uint32_t fiducials()   const { return _fiducials; }
        uint32_t frameType()   const { return _frameType; }

      public:
        FirstWordBits         first;         // 0
        SecondWordBits        second;        // 1
        uint32_t              _frameNumber;   // 2
        uint32_t              _ticks;
        uint32_t              _fiducials;
        uint16_t              _sbtemp[4];
        uint32_t              _frameType;     // 7
    };

  }
}

#endif /* DATAIMPORTFRAME_HH_ */
