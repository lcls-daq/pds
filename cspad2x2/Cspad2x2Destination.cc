/*
 * Cspad2x2Destination.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include <stdio.h>
#include "pds/pgp/RegisterSlaveImportFrame.hh"

const char* Pds::CsPad2x2::Cspad2x2Destination::name()
{
  static const char* _names[NumberOf + 1] = {
      "Quad",
      "Concentrator",
      "--INVALID--"
  };
  switch (_dest) {
    case Q0:
      return _names[0];
      break;
    case CR:
      return _names[1];
      break;
    default:
      return _names[2];
  }
}
