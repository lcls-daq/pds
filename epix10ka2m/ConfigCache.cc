#include "pds/epix10ka2m/ConfigCache.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/epix10ka2m/Server.hh"
#include "pds/epix10ka2m/FrameBuilder.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

using namespace Pds;

enum {StaticALlocationNumberOfConfigurationsForScanning=100};

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

Epix10ka2m::ConfigCache::ConfigCache(const Src& src) :
  CfgCache(src, __type(src),
           StaticALlocationNumberOfConfigurationsForScanning * __size(src)) 
{
}

void Epix10ka2m::ConfigCache::printCurrent() {
  printf("%s current %p\n", __PRETTY_FUNCTION__, current());
}

int Epix10ka2m::ConfigCache::configure(const std::vector<Pds::Epix10ka2m::Server*>& srv)
{
  int _result = 0, result;
  const DetInfo& info = static_cast<const DetInfo&>(_config.src());
  switch(info.device()) {
  case DetInfo::Epix10ka2M  : 
    { const Epix10ka2MConfigType* c = reinterpret_cast<const Epix10ka2MConfigType*>(current());
      for(unsigned q=0; q<srv.size(); q++) 
        if ((result = srv[q]->configure( c->evr(), c->quad(q), const_cast<Epix::Config10ka*>(&c->elemCfg(q*4)) ))) {
          _result |= result;
        };
    } break;
  case DetInfo::Epix10kaQuad:
    { const Epix10kaQuadConfigType* c = reinterpret_cast<const Epix10kaQuadConfigType*>(current());
      if ((result = srv[0]->configure( c->evr(), c->quad(), const_cast<Epix::Config10ka*>(&c->elemCfg(0)) ))) {
        _result |= result;
      };
    } break;
  default:
    break;
  }
  return _result;
}

void Epix10ka2m::ConfigCache::reformat(const Xtc* in, Xtc* out) const
{
  const DetInfo& info = static_cast<const DetInfo&>(_config.src());
  switch(info.device()) {
  case DetInfo::Epix10ka2M:
    { FrameBuilder(in, out,
                   *reinterpret_cast<const Epix10ka2MConfigType*>(current()));
    } break;
  case DetInfo::Epix10kaQuad:
    { FrameBuilder(in, out,
                   *reinterpret_cast<const Epix10kaQuadConfigType*>(current()));
    } break;
  default:
    break;
  }
}

int  Epix10ka2m::ConfigCache::_size(void*) const { return __size(_config.src()); }
