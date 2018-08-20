#include "pds/picam/PimaxManager.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pds/config/PimaxConfigType.hh"
#include "pds/picam/PimaxConfigWrapper.hh"
#include "pds/picam/PimaxServer.hh"

using std::string;

namespace Pds
{

PimaxManager::PimaxManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest,
  string sConfigDb, int iSleepInt, int iDebugLevel) :
  PicamManager(cfg, new PimaxConfigWrapper(cfg), iCamera, bDelayMode, bInitTest, sConfigDb, iSleepInt, iDebugLevel)
{
  try
  {
    setServer(new PimaxServer(iCamera, bDelayMode, bInitTest, cfg.src(), sConfigDb, iSleepInt, iDebugLevel));
  }
  catch ( PicamServerException& eServer )
  {
    throw PicamManagerException( "PicamManager::PicamManager(): Server Initialization Failed" );
  }
}

PimaxManager::~PimaxManager()
{
}

} //namespace Pds
