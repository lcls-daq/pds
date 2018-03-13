/*
 * Epix100aDestination.cc
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#include "pds/epix100a/Epix100aDestination.hh"
#include <stdio.h>

using namespace Pds::Epix100a;

const char* Epix100aDestination::name()
{
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "Oscilloscope",
      "--INVALID--"
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
