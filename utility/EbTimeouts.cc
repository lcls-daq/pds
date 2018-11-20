#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "EbTimeouts.hh"
#include "EbTimeoutConstants.hh"

//#define NO_TMO

using namespace Pds;

//#ifdef BUILD_PRINCETON
//static const int framework_tmo = 1000;
//static const int occurence_tmo = 1000; // 200 ms
//#else
//static const int framework_tmo = 500;
//static const int occurence_tmo = 200; // 200 ms
//#endif
static const int framework_tmo = 500;
static const int occurence_tmo = 200; // 200 ms


EbTimeouts::EbTimeouts(const EbTimeouts& ebtimeouts, int iSlowEb)
  : _duration(ebtimeouts._duration),
//#ifdef BUILD_PRINCETON
//    _tmos(60)
//#else
//    _tmos(2)
//#endif
    _tmos((iSlowEb & SlowReadout) ? EB_TIMEOUT_SLOW : EB_TIMEOUT)
{
  switch(iSlowEb & SlowReadoutAndConfig) {
    case SlowReadout:
      _tmos_config = EB_TIMEOUT_SLOW;
      break;
    case SlowConfig:
      _tmos_config = EB_TIMEOUT_SLOW_CONFIG;
      break;
    case SlowReadoutAndConfig:
      // pick the longer of EB_TIMEOUT_SLOW and EB_TIMEOUT_SLOW_CONFIG
      _tmos_config = EB_TIMEOUT_SLOW > EB_TIMEOUT_SLOW_CONFIG ? EB_TIMEOUT_SLOW : EB_TIMEOUT_SLOW_CONFIG;
      break;
    default:
      _tmos_config = EB_TIMEOUT;
      break;
  }
}

EbTimeouts::EbTimeouts(int stream,
           Level::Type level,
           int iSlowEb)
{
  if (stream == StreamParams::FrameWork) {
    _duration = framework_tmo;
  } else {
    _duration = occurence_tmo;
  }

  short extra;
  switch(level) {
   case Level::Source : extra = ((iSlowEb & SlowReadout) ? 0 : 2); break;
   case Level::Segment: extra = ((iSlowEb & SlowReadout) ? 2 : 3); break;
   case Level::Event  : extra = ((iSlowEb & SlowReadout) ? 4 : 4); break;
   case Level::Control: extra = ((iSlowEb & SlowReadout) ? 6 : 5); break;
   default            : extra = ((iSlowEb & SlowReadout) ? 6 : 5); break;
  }


  switch(iSlowEb & SlowReadoutAndConfig) {
    case SlowReadout:
      _tmos        = extra + EB_TIMEOUT_SLOW;
      _tmos_config = extra + EB_TIMEOUT_SLOW;
      break;
    case SlowConfig:
      _tmos        = extra + EB_TIMEOUT;
      _tmos_config = extra + EB_TIMEOUT_SLOW_CONFIG;
      break;
    case SlowReadoutAndConfig:
      // pick the longer of EB_TIMEOUT_SLOW and EB_TIMEOUT_SLOW_CONFIG
      _tmos        = extra + EB_TIMEOUT_SLOW;
      _tmos_config = extra + (EB_TIMEOUT_SLOW > EB_TIMEOUT_SLOW_CONFIG ? EB_TIMEOUT_SLOW : EB_TIMEOUT_SLOW_CONFIG);
      break;
    default:
      _tmos        = extra + EB_TIMEOUT;
      _tmos_config = extra + EB_TIMEOUT;
      break;
  }

//  switch(level) {
//#ifdef BUILD_PRINCETON
//    case Level::Source : _tmos = 60; break;
//    case Level::Segment: _tmos = 61; break;
//    case Level::Event  : _tmos = 62; break;
//    case Level::Control: _tmos = 63; break;
//    default            : _tmos = 63; break;
//#elif defined(BUILD_READOUT_GROUP)
//    case Level::Source : _tmos = 4; break;
//    case Level::Segment: _tmos = 5; break;
//    case Level::Event  : _tmos = 6; break;
//    case Level::Control: _tmos = 7; break;
//    default            : _tmos = 7; break;
//#else
//    case Level::Source : _tmos = 1; break;
//    case Level::Segment: _tmos = 2; break;
//    case Level::Event  : _tmos = 3; break;
//    case Level::Control: _tmos = 4; break;
//    default            : _tmos = 4; break;
//#endif
//  }

  //printf("EbTimeouts::EbTimeouts(stream = %d, level = %s, iSlowEb = %d) tmo = %d\n",
  //  stream, Level::name(level), iSlowEb, _tmos);//!!!debug

}

unsigned EbTimeouts::duration() const {
  return _duration;
}

unsigned EbTimeouts::duration(int s) {
  return (s == StreamParams::FrameWork) ? framework_tmo : occurence_tmo;
}

int EbTimeouts::timeouts(const Sequence* sequence) const {

#ifdef NO_TMO
  return INT_MAX;
#endif

  //printf("EbTimeouts::timeouts() tmo = %d\n", _tmos);//!!!debug
  if (sequence && sequence->service()==TransitionId::Configure)
    return _tmos_config;
  else
    return _tmos;
//#if defined(BUILD_PRINCETON) || defined(BUILD_READOUT_GROUP)
//  // No quick timeout for L1Accept
//#else
//  if (sequence && sequence->service()==TransitionId::L1Accept)
//    return 1;
//#endif

//  return _tmos;
}

void EbTimeouts::dump() const
{
}
