#include "pds/pvdaq/WaveformPvServer.hh"
#include "hsd/Event.hh"

#include <stdio.h>
#include <string.h>

static const size_t elem_sz = sizeof(uint16_t);
static const size_t ca_elem_sz = sizeof(dbr_long_t);
static const size_t hdr_sz = (sizeof(Pds::HSD::EventHeader) + ca_elem_sz -1) / ca_elem_sz * ca_elem_sz;
static const size_t hdr_elem = hdr_sz / ca_elem_sz;
static const size_t strm_sz = (sizeof(Pds::HSD::StreamHeader) + ca_elem_sz -1) / ca_elem_sz * ca_elem_sz;
static const size_t strm_elem = strm_sz / ca_elem_sz;

using namespace Pds::PvDaq;

AcqirisPvServer::AcqirisPvServer(const char* name, Pds_Epics::PVMonitorCb* mon_cb, const unsigned nelems) :
  Pds_Epics::EpicsCA (name, mon_cb, nelems),
  _name(new char[strlen(name)+1])
{
  strcpy(_name, name);
}

AcqirisPvServer::~AcqirisPvServer()
{
    delete[] _name;
}

const char* AcqirisPvServer::name() const
{
  return _name;
}

int AcqirisPvServer::fetch(void* payload, size_t len)
{
#ifdef DBUG
  printf("ConfigServer[%s] fetch %p\n",_channel.epicsName(),payload);
#endif
  int result = 0;
  int nelem = _channel.nelements();
  switch(_channel.type()) {
  case DBR_TIME_SHORT:
    {
      if (nelem * sizeof (dbr_short_t) > len) {
        result = -1;
      } else {
        dbr_short_t* inp = (dbr_short_t*)data();
        dbr_short_t* outp = (dbr_short_t*)payload;
        for(int k=0; k<nelem; k++) *outp++ = *inp++;
        result = (char*)outp - (char*)payload;
      }
    }
    break;
  default: printf("Unsupported type %d\n", int(_channel.type())); result=-1; break;
  }
  return result;
}


void AcqirisPvServer::update() { _channel.get(); }

QuadAdcPvServer::QuadAdcPvServer(const char* name,
                                 Pds_Epics::PVMonitorCb* mon_cb,
                                 const unsigned nchans,
                                 const unsigned elems,
                                 const double offset,
                                 const double range,
                                 const double scale,
                                 const bool sparse,
                                 const unsigned sparse_lo,
                                 const unsigned sparse_hi) :
  Pds_Epics::EpicsCA (name, mon_cb, hdr_elem + nchans * (strm_elem + elems / (ca_elem_sz / elem_sz))),
  _name(new char[strlen(name)+1]),
  _offset(offset),
  _range(range),
  _scale(scale),
  _sparse(sparse),
  _fill((sparse_lo + sparse_hi)/2)
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
  size_t data_size = 0;
  switch(_channel.type()) {
  case DBR_TIME_LONG:
    {
      double* outp = (double*)payload;
      const Pds::HSD::EventHeader* eh = reinterpret_cast<const Pds::HSD::EventHeader*>(data());
      Pds::HSD::StreamIterator it = eh->streams();
      for(const Pds::HSD::StreamHeader* sh = it.first(); sh; sh=it.next()) {
        data_size += sizeof(double) * sh->num_samples();
        if (data_size > len)
          return -1;
        const uint16_t *inp = sh->data();
        for(unsigned k=0; k<sh->num_samples(); k++)
          if (_sparse && (*inp & 0x8000)) {
            for (unsigned l=0; l<(*inp & 0x7fff); l++) {
              *outp++ = (_fill - _offset) / _range * _scale;
            }
            inp++;
          } else {
            *outp++ = (*inp++ - _offset) / _range * _scale;
          }
      }
      result = (char*)outp - (char*)payload;
    }
    break;
  default: printf("Unsupported type %d\n", int(_channel.type())); result=-1; break;
  }
  return result;
}

void QuadAdcPvServer::update() { _channel.get(); }
