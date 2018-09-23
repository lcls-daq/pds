#include "pds/config/UxiDataType.hh"

void Pds::UxiData::setFrameInfo(UxiDataType& d,
                                unsigned a,
                                unsigned t)
{
  new (&d) UxiDataType(a,
                       t,
                       d.temperature());
}

void Pds::UxiData::setTemperature(UxiDataType& d,
                                  double t)
{
  new (&d) UxiDataType(d.acquisitionCount(),
                       d.timestamp(),
                       t);
}
