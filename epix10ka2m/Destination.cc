/*
 * Epix10kaDestination.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include "pds/epix10ka2m/Destination.hh"
#include <stdio.h>

using namespace Pds::Epix10ka2m;

const char* Destination::name()
{
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "Oscilloscope",
      "Monitor"
  };
  switch (_dest) {
    case Data:
      return _names[0];
      break;
    case Registers:
      return _names[1];
      break;
    case Oscilloscope:
      return _names[2];
      break;
    default:
      return _names[3];
  }
}
