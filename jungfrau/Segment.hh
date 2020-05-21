#ifndef Pds_Jungfrau_Segment_hh
#define Pds_Jungfrau_Segment_hh

#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/JungfrauConfigType.hh"
#include "pds/config/JungfrauDataType.hh"
#include <map>

namespace Pds {
  class Xtc;
  namespace Jungfrau {
    class DamageHelper {
    public:
      static void merge(Damage& dest, const Damage& src);
    };

    class SegmentConfig {
    public:
      SegmentConfig(const DetInfo& info,
                    const Damage& damage,
                    const JungfrauConfigType& config,
                    bool transient=false);
      ~SegmentConfig();

      const DetInfo& info() const;
      const DetInfo& parent() const;
      const JungfrauConfigType& config() const;
      unsigned index() const;
      unsigned detSize() const;
      bool verify(const JungfrauConfigType& config) const;
      bool transient() const;
      Damage damage() const;
    private:
      const DetInfo            _info;
      const Damage             _damage;
      const JungfrauConfigType _config;
      const DetInfo            _parent;
      bool                     _transient;
    };

    class FrameCache {
    public:
      FrameCache(const JungfrauConfigType& config);
      ~FrameCache();
      const uint32_t size() const;
      const JungfrauDataType* frame() const;
      void insert(const SegmentConfig* seg_cfg, const JungfrauDataType* seg, const Damage& dmg);
      bool ready() const;
      void reset();
      Damage damage() const;
    private:
      const uint32_t          _size;
      const uint32_t          _mod_size;
      const uint32_t          _nmods;
      uint32_t                _mask;
      Damage                  _damage;
      char*                   _buffer;
      uint16_t*               _data;
      JungfrauDataType*       _frame;
      JungfrauModuleInfoType* _mods;
    };

    typedef std::multimap<DetInfo, SegmentConfig*> MultiSegmentMap;
    typedef MultiSegmentMap::const_iterator MultiSegmentIter;
    typedef std::pair<MultiSegmentIter, MultiSegmentIter> MultiSegmentRange;
    typedef std::map<DetInfo, SegmentConfig*> SegmentMap;
    typedef SegmentMap::const_iterator SegmentIter;
    typedef std::map<DetInfo, FrameCache*> FrameCacheMap;
    typedef FrameCacheMap::iterator FrameCacheIter;
    typedef std::map<DetInfo, DetInfo> ParentMap;
    typedef ParentMap::const_iterator ParentIter;
    typedef std::map<DetInfo, bool> TransientMap;
    typedef TransientMap::const_iterator  TransientIter;
  }
}
#endif
