#ifndef Pds_Jungfrau_DataFormat_hh
#define Pds_Jungfrau_DataFormat_hh

#define JF_EVENTS_TO_BUFFER 120
#define JF_PACKET_NUM 128
#define JF_DATA_ELEM 4096
#define JF_FRAME_WAIT_EXIT -1

namespace Pds {
  namespace Jungfrau {

#pragma pack(push)
#pragma pack(2)
    struct jungfrau_header {
      char emptyheader[6];
      uint64_t framenum;
      uint32_t exptime;
      uint32_t packetnum;
      uint64_t bunchid;
      uint64_t timestamp;
      uint16_t moduleID;
      uint16_t xCoord;
      uint16_t yCoord;
      uint16_t zCoord;
      uint32_t debug;
      uint16_t roundRobin;
      uint8_t detectortype;
      uint8_t headerVersion;
    };

    struct jungfrau_dgram {
      struct jungfrau_header header;
      uint16_t data[JF_DATA_ELEM];
    };
#pragma pack(pop)

  }
}
#endif
