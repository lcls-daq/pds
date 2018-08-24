#include "pds/config/PixisDataType.hh"

void Pds::PixisData::setTemperature(PixisDataType& d,
                                    float          t)
{
  new (&d) PixisDataType(d.shotIdStart(),
                         d.readoutTime(),
                         t);
}
