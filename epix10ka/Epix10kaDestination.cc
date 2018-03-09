/*
 * Epix10kaDestination.cc
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#include "pds/epix10ka/Epix10kaDestination.hh"
#include <stdio.h>

using namespace Pds::Epix10ka;

const char* Epix10kaDestination::name()
{
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "Oscilloscope",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
