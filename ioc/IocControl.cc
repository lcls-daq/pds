#include "pds/ioc/IocControl.hh"
#include "pds/ioc/IocNode.hh"
#include "pds/ioc/IocConnection.hh"
#include "pds/ioc/IocHostCallback.hh"
#include "pds/utility/Occurrence.hh"
#include<unistd.h>
#include<string>
#include<fstream>  //for std::ifstream
#include<sstream>  //for std::istringstream
#include<iterator> //for std::istream_iterator
#include<iostream>
#include<vector>

using namespace Pds;

IocControl::IocControl() :
  _station(0),
  _expt_id(0),
  _recording(0),
  _pool      (sizeof(UserMessage),4)
{
}

IocControl::IocControl(const char* offlinerc,
		       const char* instrument,
		       unsigned    station,
		       unsigned    expt_id,
                       const char* controlrc) :
  _instrument(instrument),
  _station   (station),
  _expt_id   (expt_id),
  _recording (0),
  _pool      (sizeof(UserMessage),4)
{
    std::ifstream *in;
    std::string line, host;

    /*
     * If we don't have a configuration file, we're not doing anything!
     */
    if (!controlrc)
        return;

    /*
     * Read in the logbook parameters.
     */
    in = new std::ifstream(offlinerc);
    if (!in) {
        _report_error("IocControl: Cannot open logbook credential file " +
                      (std::string) offlinerc);
        return;
    }
    while (getline(*in, line)) {
        _offlinerc.push_back("logbook " + line + "\n");
    }
    delete in;

    /*
     * Read the control recorder configuration file.  All we really
     * want here are the host and camera lines.  Everything else is
     * GUI, BLD, or PV-related.
     */
    in = new std::ifstream(controlrc);
    if (!in) {
        _report_error("IocControl: Cannot open controls configuration file " +
                      (std::string) controlrc);
        return;
    }
    while (getline(*in, line)) {
        std::istringstream ss(line);
        std::istream_iterator<std::string> begin(ss), end;
        std::vector<std::string> arrayTokens(begin, end);

        if (arrayTokens.size() == 0)
            continue;
        if (arrayTokens[0] == "host" && arrayTokens.size() >= 2) {
            host = arrayTokens[1];
        } else if (arrayTokens[0] == "camera" && arrayTokens.size() >= 5) {
            /* Skip the cameras that want to be recorded as BLD. */
            if (arrayTokens[2].compare(0, 4, "BLD-"))
                _nodes.push_back(new IocNode(host, line, arrayTokens[1], arrayTokens[2],
                                             arrayTokens[3], arrayTokens[4],
                                             arrayTokens.size() >= 6 ? arrayTokens[5]
                                                                     : (std::string)""));
        }
    }
    delete in;
}

IocControl::~IocControl()
{
    IocConnection::clear_all();
    _nodes.clear();
    _selected_nodes.clear();
}

void IocControl::write_config(IocConnection *c, unsigned run, unsigned stream)
{
    char buf[1024];

    c->transmit("hostname " + c->host() + "\n");
    sprintf(buf, "dbinfo daq %d %d %d\n", _expt_id, run, stream);
    c->transmit(buf);
    for(std::list<std::string>::iterator it=_offlinerc.begin();
        it!=_offlinerc.end(); it++)
        c->transmit(*it);
    sprintf(buf, "output daq/e%03d-r%04d-s%02d\n", _expt_id, run, stream);
    c->transmit(buf);
    c->transmit("quiet\n");
    c->transmit(_trans);
}

void IocControl::host_rollcall(IocHostCallback* cb)
{
  if (cb) {
    for(std::list<IocNode*>::iterator it=_nodes.begin();
	it!=_nodes.end(); it++)
      cb->connected(*(*it));
  }
}

void IocControl::set_partition(const std::list<DetInfo>& iocs)
{
  _selected_nodes.clear();
  for(std::list<DetInfo>::const_iterator it=iocs.begin();
      it!=iocs.end(); it++)
    for(std::list<IocNode*>::iterator nit=_nodes.begin();
	nit!=_nodes.end(); nit++)
      if (it->phy() == (*nit)->src().phy())
	_selected_nodes.push_back(*nit);
}

Transition* IocControl::transitions(Transition* tr)
{
  switch(tr->id()) {
  case TransitionId::Configure:
      /* Make connections to the world, and send them our configuration. */
      for(std::list<IocNode*>::iterator it=_selected_nodes.begin();
          it!=_selected_nodes.end(); it++) {
          IocConnection *c = (*it)->get_connection(this);
          (*it)->write_config(c);
      }
      sprintf(_trans, "trans %d %d %d %d\n",
              TransitionId::Configure, tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      break;
  case TransitionId::BeginRun: 
      if (tr->size() != sizeof(Transition)) {
          unsigned run = tr->env().value();
          unsigned stream = 80;

          for(std::list<IocConnection*>::iterator it=IocConnection::_connections.begin();
              it!=IocConnection::_connections.end(); it++) {
              write_config(*it, run, stream);
              stream++;
          }
          _recording = 1;
      } else {
          break;
      }
      /* FALL THROUGH!!! */
  case TransitionId::BeginCalibCycle:
  case TransitionId::Enable:
  case TransitionId::Disable:
  case TransitionId::EndCalibCycle:
      if (!_recording)
          break;
      sprintf(_trans, "trans %d %d %d %d\n",
              tr->id(), tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      IocConnection::transmit_all(_trans);
      break;
  case TransitionId::EndRun:
      if (!_recording)
          break;
      /* Pass along the transition, and then tear down all of the connections. */
      sprintf(_trans, "trans %d %d %d %d\n",
              tr->id(), tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      IocConnection::transmit_all(_trans);
      IocConnection::clear_all();
      _recording = 0;
      break;
  default:
      break;
  }
  return tr;
}

void IocControl::_report_error(const std::string& msg)
{
  UserMessage* usr = new(&_pool) UserMessage(msg.c_str());
  post(usr);
}