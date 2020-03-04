#include "DetectorId.hh"

using namespace Pds;
using namespace Pds::Jungfrau;

static const uint64_t MOD_ID_BITS = 16;
static const uint64_t MOD_ID_MASK = (1<<MOD_ID_BITS) - 1;

static const DetIdItem DEFAULTS[] = {
  DetIdItem("det-jungfrau4m-00", DetId(0x0050c246df50, 269)),
  DetIdItem("det-jungfrau4m-01", DetId(0x0050c246df51, 279)),
  DetIdItem("det-jungfrau4m-02", DetId(0x0050c246df53, 282)),
  DetIdItem("det-jungfrau4m-03", DetId(0x0050c246df54, 296)),
  DetIdItem("det-jungfrau4m-04", DetId(0x0050c246df55, 277)),
  DetIdItem("det-jungfrau4m-05", DetId(0x0050c246df56, 287)),
  DetIdItem("det-jungfrau4m-06", DetId(0x0050c246df57, 289)),
  DetIdItem("det-jungfrau4m-07", DetId(0x0050c246df58, 254)),
};

DetId::DetId() :
  _id(0)
{}

DetId::DetId(uint64_t id) :
  _id(id)
{}

DetId::DetId(uint64_t board, uint64_t module) :
  _id((board<<MOD_ID_BITS) | (MOD_ID_MASK & module))
{}

DetId::~DetId()
{}

uint64_t DetId::full() const
{
  return _id;
}

 uint64_t DetId::board() const
{
  return _id>>MOD_ID_BITS;
}

uint64_t DetId::module() const
{
  return MOD_ID_MASK & _id;
}

DetIdLookup::DetIdLookup()
{
  for(unsigned i=0; i<sizeof(DEFAULTS)/sizeof(DEFAULTS[0]); i++)
    _lookup.insert(DEFAULTS[i]);
}

DetIdLookup::~DetIdLookup()
{}

bool DetIdLookup::has(const std::string& hostname) const
{
  DetIdIter it = _lookup.find(hostname);
  return it != _lookup.end();
}

const DetId& DetIdLookup::operator[](const std::string& hostname)
{
  return _lookup[hostname];
}
