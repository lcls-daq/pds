#include "pds/epix10ka2m/ServerSim.hh"
#include "pds/epix10ka2m/Configurator.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/xtc/InDatagram.hh"

using namespace Pds::Epix10ka2m;

ServerSim::ServerSim( Pds::Epix10ka2m::Server*  hdw ) : _hdw(hdw), _bwd(0)  , _fwd(0) 
{
  printf("%s\n",__PRETTY_FUNCTION__);
  fd(hdw->fd());
}

ServerSim::ServerSim( ServerSim*      bwd ) : _hdw(0)  , _bwd(bwd), _fwd(0) 
{
  printf("%s\n",__PRETTY_FUNCTION__);
  bwd->_fwd = this; 
  pipe(_pfd);
  fd(_pfd[0]);
}

void ServerSim::dump(int) const {}

bool ServerSim::isValued() const { return true; }

#define DEFER(f) (_hdw ? _hdw->f() : _bwd->f())

const Pds::Src& ServerSim::client(void) const    { return DEFER(client); }
const Pds::Xtc& ServerSim::xtc   (void) const    { return DEFER(xtc); }
unsigned        ServerSim::length(    ) const    { return DEFER(length); }

int ServerSim::pend( int flag ) { return -1; }

int ServerSim::fetch( char* payload, int flags )
{
  int payloadSize;
  if (_hdw) {
    payloadSize = _hdw->fetch(payload,flags);
    if (_fwd && payloadSize>0) {
      _fwd->post(payload,payloadSize);
    }
  }
  else {
    char* src;
    read(_pfd[0], &src        , sizeof(src));
    read(_pfd[0], &payloadSize, sizeof(int));
    memcpy(payload, src, payloadSize);

    Pds::Pgp::DataImportFrame* data = reinterpret_cast<Pds::Pgp::DataImportFrame*>(payload+2*sizeof(Pds::Xtc));
    data->first.lane++;

    if (_fwd) {
      _fwd->post(payload,payloadSize);
    }
  }
  Pds::Pgp::DataImportFrame* data = reinterpret_cast<Pds::Pgp::DataImportFrame*>(payload+2*sizeof(Pds::Xtc));
  _fiducials = data->fiducials();
  return payloadSize;
}

bool ServerSim::more() const { return false; }

unsigned ServerSim::fiducials() const { return _fiducials; }

void     ServerSim::allocated  (const Allocate& a)
{ if (_hdw) _hdw->allocated(a); }

unsigned ServerSim::configure(const Pds::Epix::PgpEvrConfig& evr,
                              const Epix10kaQuadConfig& cfg, 
                              Pds::Epix::Config10ka* elem,
                              bool forceConfig)
{
  unsigned result =  _hdw ? _hdw->configure(evr,cfg,elem,forceConfig) : 0;
  //  Set the ASIC mask so analysis will process
  for(unsigned i=0; i<4; i++) {
    uint32_t* u = reinterpret_cast<uint32_t*>(&elem[i]);
    u[2] = 0xf; 
  }
  return result;
}

unsigned ServerSim::unconfigure(void)
{ return _hdw ? _hdw->unconfigure() : 0; }

void     ServerSim::enable     () { if (_hdw) _hdw->enable(); }
void     ServerSim::disable    () { if (_hdw) _hdw->disable(); }
bool     ServerSim::g3sync     () const { return DEFER(g3sync); }
void     ServerSim::ignoreFetch(bool b) { if (_hdw) _hdw->ignoreFetch(b); }
Configurator* ServerSim::configurator() { return _hdw ? _hdw->configurator() : 0; }
void     ServerSim::dumpFrontEnd() { if (_hdw) _hdw->dumpFrontEnd(); }
void     ServerSim::die() { if (_hdw) _hdw->die(); }

void     ServerSim::post(char* payload, int payloadSize)
{
  if (_hdw) return;
  write(_pfd[1], &payload    , sizeof(payload));
  write(_pfd[1], &payloadSize, sizeof(payloadSize));
}

void ServerSim::recordExtraConfig(InDatagram* in) const
{
  // if (_samplerConfig)
  //   in->insert(_xtcConfig, _samplerConfig);
}
