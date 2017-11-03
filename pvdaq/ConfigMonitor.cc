#include "pds/pvdaq/ConfigMonitor.hh"
#include "pds/pvdaq/Server.hh"

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

ConfigMonitor::ConfigMonitor(Server& server) :
  _server(server)
{}

ConfigMonitor::~ConfigMonitor()
{}

void ConfigMonitor::updated()
{
  _server.config_updated();
}

ConfigServer::ConfigServer(const char* name, ConfigMonitor* cfgmon) :
  Pds_Epics::EpicsCA (name, cfgmon),
  _name(new char[strlen(name)+1])
{
  strcpy(_name, name);
}

ConfigServer::~ConfigServer()
{
  delete[] _name;
}

const char* ConfigServer::name() const
{
  return _name;
}

int ConfigServer::fetch_str(void* payload, size_t len)
{
 #ifdef DBUG
  printf("ConfigServer[%s] fetch_str %p\n",_channel.epicsName(),payload);
#endif
  int result = 0;
  int nelem = _channel.nelements();
  if (_channel.type() == DBR_TIME_STRING) {
    if (nelem * sizeof (dbr_string_t) > len) {
      result=-1;
    } else {
      dbr_string_t* inp  = (dbr_string_t*) data();
      dbr_string_t* outp = (dbr_string_t*) payload;
      for(int k=0; k<nelem; k++) {
        memcpy(outp++, inp++, sizeof(dbr_string_t));
      }
      result = (char*)outp - (char*)payload;
    }
  } else {
    printf("Fetch string called on non-string type %d\n", int(_channel.type()));
    result=-1;
  }

  return result;
}

int ConfigServer::fetch(void* payload, size_t len)
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

void ConfigServer::update() { _channel.get(); }

#undef handle_type
