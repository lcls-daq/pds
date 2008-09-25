#ifndef PDS_STREAMPORTS_HH
#define PDS_STREAMPORTS_HH

#include "pds/service/Ins.hh"
#include "pds/collection/Level.hh"
#include "StreamParams.hh"

//
//  We build events from two types of data:
//    (1) fragments that vector event data within its partition and
//    (2) fragments that flood event data to all partitions
//
namespace PdsStreamPorts {
  Pds::Ins event     (unsigned         partition,
		      Pds::Level::Type level,
		      unsigned         dstid=0,
		      unsigned         srcid=0);
  Pds::Ins bld       (unsigned         id);
}

namespace Pds {
  class StreamPorts {
  public:
    StreamPorts();
    StreamPorts(const StreamPorts&);

    void           port(unsigned stream, unsigned short);
    unsigned short port(unsigned stream) const;
  private:
    unsigned short _ports[StreamParams::NumberOfStreams];
  };
}

#endif
