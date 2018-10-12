/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : Manager.hh
-- Author     : Matt Weaver <weaver@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2018-09-28
-- Last update: 2018-09-17
-- Platform   : 
-- Standard   : C++
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- This file is part of 'LCLS DAQ'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'LCLS DAQ', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------*/

#ifndef Pds_Epix10ka2m_Manager_hh
#define Pds_Epix10ka2m_Manager_hh

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"

#include <vector>

namespace Pds {

  namespace Epix10ka2m {
    class Server;
    class ConfigCache;

    class Manager {
    public:
      Manager(const std::vector<Server*>&);
      Appliance& appliance( void ) { return _fsm; }
      
    private:
      Fsm& _fsm;
      ConfigCache& _cfg;
    };
  };
};

#endif
