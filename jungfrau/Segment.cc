#include "Segment.hh"

#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/SegmentInfo.hh"

using namespace Pds;
using namespace Pds::Jungfrau;

void DamageHelper::merge(Damage& dest, const Damage& src)
{
  dest.increase(src.value());
  dest.userBits(src.userBits());
}

SegmentConfig::SegmentConfig(const DetInfo& info,
                             const Damage& damage,
                             const JungfrauConfigType& config) :
  _info(info),
  _damage(damage),
  _config(config),
  _parent(SegmentInfo::parent(info))
{}

SegmentConfig::~SegmentConfig()
{}

const DetInfo& SegmentConfig::info() const
{
  return _info;
}

const DetInfo& SegmentConfig::parent() const
{
  return _parent;
}

const JungfrauConfigType& SegmentConfig::config() const
{
  return _config;
}

unsigned SegmentConfig::index() const
{
  return _info.devId() & 0xf;
}

unsigned SegmentConfig::detSize() const
{
  return _info.detId() & 0xf;
}

Damage SegmentConfig::damage() const
{
  return _damage;
}

bool SegmentConfig::verify(const JungfrauConfigType& config) const
{
  if (config.biasVoltage() != _config.biasVoltage())
    return false;
  else if (config.gainMode() != _config.gainMode())
    return false;
  else if (config.speedMode() != _config.speedMode())
    return false;
  else if (config.triggerDelay() != _config.triggerDelay())
    return false;
  else if (config.exposureTime() != _config.exposureTime())
    return false;
  else if (config.exposurePeriod() != _config.exposurePeriod())
    return false;
  else if (config.vb_ds() != _config.vb_ds())
    return false;
  else if (config.vb_comp() != _config.vb_comp())
    return false;
  else if (config.vb_pixbuf() != _config.vb_pixbuf())
    return false;
  else if (config.vref_ds() != _config.vref_ds())
    return false;
  else if (config.vref_prech() != _config.vref_prech())
    return false;
  else if (config.vin_com() != _config.vin_com())
    return false;
  else if (config.vdd_prot() != _config.vdd_prot())
    return false;
  else
    return false;
}

FrameCache::FrameCache(const JungfrauConfigType& config) :
  _size(JungfrauDataType::_sizeof(config)),
  _mod_size(config.numPixels()/config.numberOfModules()),
  _nmods((1<<config.numberOfModules())-1),
  _mask(0),
  _damage(0),
  _buffer(NULL),
  _data(NULL),
  _frame(NULL),
  _mods(NULL)
{
  _buffer = new char[_size];
  _frame = reinterpret_cast<JungfrauDataType*>(_buffer);
  _mods = reinterpret_cast<JungfrauModuleInfoType*>(_frame+1);
  _data = reinterpret_cast<uint16_t*>(_mods+config.numberOfModules());
}

FrameCache::~FrameCache()
{
  if (_buffer) {
    delete[] _buffer;
    _buffer = NULL;
  }
}

const uint32_t FrameCache::size() const
{
  return _size;
}

const JungfrauDataType* FrameCache::frame() const
{
  return _frame;
}

void FrameCache::insert(const SegmentConfig* seg_cfg, const JungfrauDataType* seg, const Damage& dmg)
{
  const JungfrauConfigType& cfg = seg_cfg->config();

  DamageHelper::merge(_damage, dmg);

  if (seg_cfg->index() == 0) {
    new(_frame) JungfrauDataType(seg->frameNumber(), seg->ticks(), seg->fiducials());
  }

  for (unsigned i=0; i<cfg.numberOfModules(); i++) {
    _mods[i+seg_cfg->index()] = seg->moduleInfo(i);
    _mask |= 1<<(i+seg_cfg->index());
  }

  memcpy(&_data[_mod_size*seg_cfg->index()],
         seg->frame(seg_cfg->config()).data(),
         _mod_size*cfg.numberOfModules()*sizeof(uint16_t));
}

bool FrameCache::ready() const
{
  return _mask == _nmods;
}

void FrameCache::reset()
{
  _mask = 0;
  _damage = 0;
}

Damage FrameCache::damage() const
{
  return _damage;
}
