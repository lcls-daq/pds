#include "pds/config/UxiConfigType.hh"

void Pds::UxiConfig::setPots(UxiConfigType& c,
                             double* p)
{
  new(&c) UxiConfigType(c.roiEnable(),
                        c.roiRows(),
                        c.roiFrames(),
                        c.width(),
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

void Pds::UxiConfig::setRoi(UxiConfigType& c,
                            unsigned fr,
                            unsigned lr,
                            unsigned ff,
                            unsigned lf)
{
  Pds::Uxi::RoiCoord roiRows(fr, lr);
  Pds::Uxi::RoiCoord roiFrames(ff, lf);
  new(&c) UxiConfigType(c.roiEnable(),
                        roiRows,
                        roiFrames,
                        c.width(),
                        c.height(),
                        c.numberOfFrames(),
                        c.numberOFBytesPerPixel(),
                        c.sensorType(),
                        c.timeOn().data(),
                        c.timeOff().data(),
                        c.delay().data(),
                        c.readOnlyPots(),
                        c.pots().data());
}

void Pds::UxiConfig::setSize(UxiConfigType& c,
                             unsigned w,
                             unsigned h,
                             unsigned n,
                             unsigned b,
                             unsigned t)
{
  new(&c) UxiConfigType(c.roiEnable(),
                        c.roiRows(),
                        c.roiFrames(),
                        w,
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
