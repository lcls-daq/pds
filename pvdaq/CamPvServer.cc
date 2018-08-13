#include "pds/pvdaq/CamPvServer.hh"

#include <stdio.h>
#include <string.h>

#define handle_type(ctype, stype, dtype) case ctype:    \
  if (nelem * sizeof (dtype) > len) {                   \
    result = -1;                                        \
  } else {                                              \
    dtype* inp  = (dtype*)data();                       \
    dtype* outp = (dtype*)payload;                      \
    for(int k=0; k<nelem; k++) *outp++ = *inp++;        \
    result = (char*)outp - (char*)payload;              \
  }

using namespace Pds::PvDaq;

ImageServer::ImageServer(const char* name, Pds_Epics::PVMonitorCb* mon_cb, const int max_elem) :
  Pds_Epics::EpicsCA (name, mon_cb, max_elem),
  _name(new char[strlen(name)+1])
{
  strcpy(_name, name);
}

ImageServer::~ImageServer()
{
  delete[] _name;
}

const char* ImageServer::name() const
{
  return _name;
}

int ImageServer::fetch(void* payload, size_t len)
{
#ifdef DBUG
  printf("ConfigServer[%s] fetch %p\n",_channel.epicsName(),payload);
#endif
  int result = 0;
  int nelem = _channel.nelements();
  switch(_channel.type()) {
    handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
    handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
    handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
    handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
    handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
    handle_type(DBR_TIME_CHAR  , dbr_time_char  , dbr_char_t  ) break;
  default: printf("Unknown type %d\n", int(_channel.type())); result=-1; break;
  }
  return result;
}

void ImageServer::update() { _channel.get(); }


#undef handle_type
