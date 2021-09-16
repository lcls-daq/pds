#include "ConfigCache.hh"
#include "AlviumConfigCache.hh"
#include "Driver.hh"

#include "pds/config/VimbaConfigType.hh"
#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/DetInfo.hh"

using namespace Pds;

enum {NumberOfConfigurationsForScanning=100};

static TypeId __type(const Src& src)
{
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Alvium  : return _alviumConfigType;
  default: break;
  }
  return TypeId(TypeId::Any,0);
}

static int __size(const Src& src) { 
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Alvium  : return AlviumConfigType::_sizeof();
  default: break;
  }
  return 0;
}

Vimba::ConfigCache* Vimba::ConfigCache::create(const Src& xtc, Camera& cam)
{
  const DetInfo& info = static_cast<const DetInfo&>(xtc);
  switch (info.device()) {
  case DetInfo::Alvium  : return new AlviumConfigCache(xtc, cam);
  default: break;
  }
  return NULL;
}

Vimba::ConfigCache::ConfigCache(const Src& src, Camera& cam) :
  CfgCache(src, __type(src), NumberOfConfigurationsForScanning * __size(src)),
  _error(new char[MaxErrMsgLength]),
  _cam(cam)
{}

Vimba::ConfigCache::~ConfigCache()
{
  if (_error)
    delete[] _error;
}

const char* Vimba::ConfigCache::get_error() const
{
  return _error;
}

void Vimba::ConfigCache::clear_error() {
  memset(_error, 0, MaxErrMsgLength);
}

bool Vimba::ConfigCache::configure(bool apply)
{
  try {
    if (!_configure(apply)) {
      _configtc.damage.increase(Damage::UserDefined);
      _configtc.damage.userBits(0x1);
      return false;
    } else {
      return true;
    }
  } catch(VimbaException& e) {
      fprintf(stderr, "VimbaConfig: exception encountered configuring camera!\n");
      snprintf(_error, MaxErrMsgLength,
               "Vimba Config: exception encountered configuring camera %s: %s!",
               DetInfo::name(static_cast<const DetInfo&>(_config.src())),
               e.what());
    _configtc.damage.increase(Damage::UserDefined);
    _configtc.damage.userBits(0x2);
    return false;
  }
}

int Vimba::ConfigCache::_size(void*) const { return __size(_config.src()); }
