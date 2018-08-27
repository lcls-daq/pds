#include "pds/picam/PixisManager.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pds/config/PixisConfigType.hh"
#include "pds/picam/PixisConfigWrapper.hh"
#include "pds/picam/PixisServer.hh"

using std::string;

namespace Pds
{

PixisManager::PixisManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest,
  string sConfigDb, int iSleepInt, int iDebugLevel) :
  PicamManager(cfg, new PixisConfigWrapper(cfg), iCamera, bDelayMode, bInitTest, sConfigDb, iSleepInt, iDebugLevel)
{
  try
  {
    setServer(new PixisServer(iCamera, bDelayMode, bInitTest, cfg.src(), sConfigDb, iSleepInt, iDebugLevel));
  }
  catch ( PicamServerException& eServer )
  {
    throw PicamManagerException( "PicamManager::PicamManager(): Server Initialization Failed" );
  }
}

PixisManager::~PixisManager()
{
}

} //namespace Pds
