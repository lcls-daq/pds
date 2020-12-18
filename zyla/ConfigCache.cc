#include "ConfigCache.hh"
#include "ZylaConfigCache.hh"
#include "iStarConfigCache.hh"
#include "Driver.hh"

#include "pds/config/ZylaConfigType.hh"
#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/DetInfo.hh"

using namespace Pds;

enum {NumberOfConfigurationsForScanning=100};

static TypeId __type(const Src& src)
{
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Zyla  : return _zylaConfigType;
  case DetInfo::iStar : return _istarConfigType;
  default: break;
  }
  return TypeId(TypeId::Any,0);
}

static int __size(const Src& src) { 
  const DetInfo& info = static_cast<const DetInfo&>(src);
  switch(info.device()) {
  case DetInfo::Zyla  : return ZylaConfigType::_sizeof();
  case DetInfo::iStar : return iStarConfigType::_sizeof();
  default: break;
  }
  return 0;
}

Zyla::ConfigCache* Zyla::ConfigCache::create(const Src& xtc, Driver& driver)
{
  const DetInfo& info = static_cast<const DetInfo&>(xtc);
  switch (info.device()) {
  case DetInfo::Zyla  : return new ZylaConfigCache(xtc, driver);
  case DetInfo::iStar : return new iStarConfigCache(xtc, driver);
  default: break;
  }
  return NULL;
}

Zyla::ConfigCache::ConfigCache(const Src& src, Driver& driver) :
  CfgCache(src, __type(src), NumberOfConfigurationsForScanning * __size(src)),
  _error(new char[MaxErrMsgLength]),
  _wc_buffer(new AT_WC[MaxAtMsgLength]),
  _driver(driver)
{}

Zyla::ConfigCache::~ConfigCache()
{
  if (_error)
    delete[] _error;
  if (_wc_buffer)
    delete[] _wc_buffer;
}

const char* Zyla::ConfigCache::get_error() const
{
  return _error;
}

void Zyla::ConfigCache::clear_error() {
  memset(_error, 0, MaxErrMsgLength);
}

int Zyla::ConfigCache::_size(void*) const { return __size(_config.src()); }
