#ifndef Pds_Jungfrau_DetectorId_hh
#define Pds_Jungfrau_DetectorId_hh

#include <stdint.h>
#include <string>
#include <map>

namespace Pds {
  namespace Jungfrau {
    class DetId {
    public:
      DetId();
      DetId(uint64_t id);
      DetId(uint64_t board, uint64_t module);
      ~DetId();
      uint64_t full() const;
      uint64_t board() const;
      uint64_t module() const;
    private:
      const uint64_t _id;
    };

    typedef std::map<std::string, DetId> DetIdMap;
    typedef std::pair<std::string, DetId> DetIdItem;
    typedef DetIdMap::const_iterator DetIdIter;

    class DetIdLookup {
    public:
      DetIdLookup();
      ~DetIdLookup();

      bool has(const std::string& hostname) const;
      const DetId& operator[](const std::string& hostname);
    private:
      DetIdMap _lookup;
    };
  }
}
#endif
