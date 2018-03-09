/*
 * Epix10kaManager.hh
 *
 *  Created on: 2017.10.27
 *      Author: jackp
 */

#ifndef EPIX10KAMANAGER_HH
#define EPIX10KAMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

namespace Pds {
  class Epix10kaServer;
  class Epix10kaManager;
  class CfgClientNfs;
  class Epix10kaConfigCache;
}

class Pds::Epix10kaManager {
  public:
    Epix10kaManager( Epix10kaServer* server, unsigned d);
    Appliance& appliance( void ) { return _fsm; }

  private:
    Fsm& _fsm;
    Epix10kaConfigCache& _cfg;
};

#endif
