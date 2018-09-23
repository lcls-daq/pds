#include "pds/config/UxiConfigType.hh"

void Pds::UxiConfig::setPots(UxiConfigType& c,
                             double* p)
{
  new(&c) UxiConfigType(c.width(),
                        c.height(),
                        c.numberOfFrames(),
                        c.numberOFBytesPerPixel(),
                        c.sensorType(),
                        c.timeOn().data(),
                        c.timeOff().data(),
                        c.delay().data(),
                        c.readOnlyPots(),
                        p);
}

void Pds::UxiConfig::setSize(UxiConfigType& c,
                            unsigned w,
                            unsigned h,
                            unsigned n,
                            unsigned b,
                            unsigned t)
{
  new(&c) UxiConfigType(w,
                        h,
                        n,
                        b,
                        t,
                        c.timeOn().data(),
                        c.timeOff().data(),
                        c.delay().data(),
                        c.readOnlyPots(),
                        c.pots().data());
}
