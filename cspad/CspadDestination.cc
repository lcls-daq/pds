/*
 * CspadDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadDestination.hh"
#include <stdio.h>

using namespace Pds::CsPad;

     const char* CspadDestination::name()
     {
        static const char* _names[NumberOf + 1] = {
         "Quad_0",
         "Quad_1",
         "Quad_2",
         "Quad_3",
         "Concentrator",
         "--INVALID--"
       };
       switch (_dest) {
         case Q0:
           return _names[0];
           break;
         case Q1:
           return _names[1];
           break;
         case Q2:
           return _names[2];
           break;
         case Q3:
           return _names[3];
           break;
         case CR:
           return _names[4];
           break;
         default:
           return _names[5];
           break;
       }
     }
