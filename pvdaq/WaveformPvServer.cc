#include "pds/pvdaq/WaveformPvServer.hh"
#include "hsd/Event.hh"

#include <stdio.h>
#include <string.h>

static const size_t elem_sz = sizeof(uint16_t);
static const size_t ca_elem_sz = sizeof(dbr_long_t);
static const size_t hdr_sz = (sizeof(Pds::HSD::EventHeader) + ca_elem_sz -1) / ca_elem_sz * ca_elem_sz;
static const size_t hdr_elem = hdr_sz / ca_elem_sz;

using namespace Pds::PvDaq;

QuadAdcPvServer::QuadAdcPvServer(const char* name, Pds_Epics::PVMonitorCb* mon_cb, const int max_elem) :
  Pds_Epics::EpicsCA (name, mon_cb, max_elem / (ca_elem_sz / elem_sz) + hdr_elem),
  _name(new char[strlen(name)+1])
{
  strcpy(_name, name);
}

QuadAdcPvServer::~QuadAdcPvServer()
{
  delete[] _name;
}

const char* QuadAdcPvServer::name() const
{
  return _name;
}

int QuadAdcPvServer::fetch(void* payload, size_t len)
{
#ifdef DBUG
  printf("ConfigServer[%s] fetch %p\n",_channel.epicsName(),payload);
#endif
  int result = 0;
  int nelem = (_channel.nelements() - hdr_elem) * (ca_elem_sz / elem_sz);
  switch(_channel.type()) {
  case DBR_TIME_LONG:
    if (nelem * sizeof (double) > len) {
      result = -1;
    } else {
      uint16_t* inp  = (uint16_t*) ((dbr_long_t*)data() + hdr_elem);
      double* outp = (double*)payload;
      for(int k=0; k<nelem; k++) *outp++ = (*inp++ - 512.)/2048.;
      result = (char*)outp - (char*)payload;
    }
    break;
  default: printf("Unsupported type %d\n", int(_channel.type())); result=-1; break;
  }
  return result;
}

void QuadAdcPvServer::update() { _channel.get(); }
