#include "pds/epix10ka2m/ConfigCache.hh"
#include "pds/epix10ka2m/Server.hh"
#include "pds/epix10ka2m/FrameBuilder.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#define USE_THREADS

using namespace Pds;

enum {StaticALlocationNumberOfConfigurationsForScanning=105};

static void updateDaqCode(Epix::PgpEvrConfig& cfg, unsigned code)
{
  new (&cfg) Epix::PgpEvrConfig(cfg.enable(),
                                cfg.runCode(),
                                (uint8_t) code,
                                cfg.runDelay());
}

static TypeId __type(const Src& src)
{
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Epix10ka2M  : return _epix10ka2MConfigType;
  case DetInfo::Epix10kaQuad: return _epix10kaQuadConfigType;
  default: break;
  }
  return TypeId(TypeId::Any,0);
}

static int __size(const Src& src) { 
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Epix10ka2M  : return Epix10ka2MConfigType::_sizeof();
  case DetInfo::Epix10kaQuad: return Epix10kaQuadConfigType::_sizeof();
  default: break;
  }
  return 0;
}

namespace Pds {
  namespace Epix10ka2m {
    class ConfigRoutine : public Routine {
    public:
      ConfigRoutine(unsigned q, Server& srv, const Epix10ka2MConfigType& c) :
        _q(q), _srv(srv), _c(c), _result(0), _sem(Semaphore::EMPTY) {}
      ~ConfigRoutine() {}
    public:
      void routine()
      {
        _result = _srv.configure( _c.evr(), _c.quad(_q), const_cast<Epix::Config10ka*>(&_c.elemCfg(_q*4)) );
        _sem.give();
      }
      unsigned result() { 
        _sem.take();
        return _result; 
      }
    private:
      unsigned _q;
      Server&  _srv;
      const Epix10ka2MConfigType& _c;
      unsigned _result;
      Semaphore _sem;
    };
  }
}

Epix10ka2m::ConfigCache::ConfigCache(const Src& src) :
  PgpCfgCache(src, __type(src),
              StaticALlocationNumberOfConfigurationsForScanning * __size(src)),
  _cache  (0)
{
  char buff[64];
  for(unsigned i=0; i<4; i++) {
    sprintf(buff,"2mCfg%u",i);
    _task   [i] = new Task(TaskObject(buff));
    _routine[i] = 0;
  }
}

Epix10ka2m::ConfigCache::~ConfigCache()
{
  for(unsigned i=0; i<4; i++) {
    _task[i]->destroy();
  }
  if (_cache)
    delete[] _cache;
}

void Epix10ka2m::ConfigCache::printCurrent() {
  printf("%s current %p\n", __PRETTY_FUNCTION__, current());
}

int Epix10ka2m::ConfigCache::configure(const std::vector<Pds::Epix10ka2m::Server*>& srv)
{
  int _result = 0, result;
  const DetInfo& info = static_cast<const DetInfo&>(_config.src());
  //  Copy the configuration into cache so readback can be recorded there
  switch(info.device()) {
  case DetInfo::Epix10ka2M  : 
    { const Epix10ka2MConfigType* c = reinterpret_cast<const Epix10ka2MConfigType*>(current());
      updateDaqCode(const_cast<Epix::PgpEvrConfig&>(c->evr()), code());
      if (_cache) delete[] _cache;
      _cache = new char[c->_sizeof()];
      memcpy(_cache,c,c->_sizeof());
      Epix10ka2MConfigType* cfg = reinterpret_cast<Epix10ka2MConfigType*>(_cache);
#ifdef USE_THREADS
      for(unsigned q=0; q<srv.size(); q++) {
        _routine[q] = new Epix10ka2m::ConfigRoutine(q, *srv[q], *cfg);
        _task[q]->call( _routine[q] );
      }
      for(unsigned q=0; q<srv.size(); q++) {
        _result |= _routine[q]->result();
        delete _routine[q];
      }
#else
      for(unsigned q=0; q<srv.size(); q++)
        _result |= srv[q]->configure( cfg->evr(), cfg->quad(q), const_cast<Epix::Config10ka*>(&cfg->elemCfg(q*4)) );
#endif
    } break;
  case DetInfo::Epix10kaQuad:
    { const Epix10kaQuadConfigType* c = reinterpret_cast<const Epix10kaQuadConfigType*>(current());
      updateDaqCode(const_cast<Epix::PgpEvrConfig&>(c->evr()), code());
      printf("***********************MY DAQ CODE IS %u\n", code());
      if (_cache) delete[] _cache;
      _cache = new char[c->_sizeof()];
      memcpy(_cache,c,c->_sizeof());
      Epix10kaQuadConfigType* cfg = reinterpret_cast<Epix10kaQuadConfigType*>(_cache);
      if ((result = srv[0]->configure( cfg->evr(), cfg->quad(), const_cast<Epix::Config10ka*>(&cfg->elemCfg(0)) ))) {
        _result |= result;
      };
    } break;
  default:
    break;
  }
  return _result;
}

void Epix10ka2m::ConfigCache::record(InDatagram* dg)
{
  if (_changed) {
    if (_configtc.damage.value())
      dg->datagram().xtc.damage.increase(_configtc.damage.value());
    else {
      //  Record configuration as it was readback
      dg->insert(_configtc, _cache);
    }
  }
}

void Epix10ka2m::ConfigCache::reformat(const Datagram& in, Datagram& out) const
{
  const DetInfo& info = static_cast<const DetInfo&>(_config.src());
  switch(info.device()) {
  case DetInfo::Epix10ka2M:
    { FrameBuilder f(in, out,
                     *reinterpret_cast<const Epix10ka2MConfigType*>(current())); 
      break; }
  case DetInfo::Epix10kaQuad:
    { FrameBuilder f(in, out,
                     *reinterpret_cast<const Epix10kaQuadConfigType*>(current()));
      break; }
  default:
    break;
  }
}

int  Epix10ka2m::ConfigCache::_size(void*) const { return __size(_config.src()); }

