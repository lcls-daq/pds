#include "pds/pvdaq/Server.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::PvDaq;

Server::Server() : _env(0), _app(0)
{
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    fd(_pfd[0]);
  }

  _occPool = new GenericPool(sizeof(Pds::UserMessage),4);
}

Server::~Server()
{
  delete _occPool;
}

void Server::setApp(Appliance& app) {
  _app = &app;
}

void Server::sendOcc(Occurrence* occ) {
  if (_app)
    _app->post(occ);
  else
    delete occ;
}

int Server::fetch( char* payload, int flags )
{
  void* p;
  int len = ::read(_pfd[0], &p, sizeof(p));
  if (len != sizeof(p)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  return fill(payload,p);
}

void Server::post(const void* p)
{
  ::write(_pfd[1], &p, sizeof(p));
}


#include "pds/pvdaq/BeamMonitorServer.hh"
#include "pds/pvdaq/ControlsCameraServer.hh"
#include "pds/pvdaq/QuadAdcServer.hh"
#include "pds/pvdaq/AcqirisServer.hh"

Server* Server::lookup(const char*    pvbase,
                       const char*    pvbase_alt,
                       const DetInfo& info,
                       const unsigned max_event_size,
                       const unsigned flags)
{
  Server* s=0;
  switch(info.device()) {
  case Pds::DetInfo::Acqiris:
    s = new AcqirisServer(pvbase,pvbase_alt,info,max_event_size,flags);
    break;
  case Pds::DetInfo::Wave8:
    s = new BeamMonitorServer(pvbase,pvbase_alt,info);
    break;
  case Pds::DetInfo::Opal1000:
  case Pds::DetInfo::Opal2000:
  case Pds::DetInfo::Opal4000:
  case Pds::DetInfo::Opal8000:
  case Pds::DetInfo::OrcaFl40:
  case Pds::DetInfo::TM6740:
  case Pds::DetInfo::Quartz4A150:
  case Pds::DetInfo::Rayonix:
  case Pds::DetInfo::ControlsCamera:
  case Pds::DetInfo::StreakC7700:
    s = new ControlsCameraServer(pvbase,pvbase_alt,info,max_event_size,flags);
    break;
  case Pds::DetInfo::QuadAdc:
    s = new QuadAdcServer(pvbase,pvbase_alt,info,max_event_size,flags);
    break;
  default:
    break;
  }
  return s;
}
