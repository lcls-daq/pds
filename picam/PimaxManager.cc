#include "pds/picam/PimaxManager.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pds/config/PimaxConfigType.hh"
#include "pds/config/PimaxDataType.hh"
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

void PimaxManager::printConfigInfo(PicamConfig* config) const
{
  PimaxConfigWrapper* pimax_config = dynamic_cast<PimaxConfigWrapper*>(config);
  if (pimax_config) {
    printf( "Pimax Config data:\n"
      "  Width %d Height %d  Org X %d Y %d  Bin X %d Y %d\n"
      "  Cooling Temperature %.1f C  Readout Speed %g MHz  Gain Index %d\n"
      "  IntGain %d  Gate delay %g ns  width %g ns\n"
      "  InfoRepInt %d  Readout Event %d  Num Integration Shots %d\n",
      pimax_config->width(), pimax_config->height(),
      pimax_config->orgX(), pimax_config->orgY(),
      pimax_config->binX(), pimax_config->binY(),
      pimax_config->coolingTemp(), pimax_config->readoutSpeed(), pimax_config->gainIndex(),
      pimax_config->intensifierGain(), pimax_config->gateDelay(), pimax_config->gateWidth(),
      pimax_config->infoReportInterval(), pimax_config->exposureEventCode(), pimax_config->numIntegrationShots()
    );
  }
}

void PimaxManager::printFrameInfo(void* payload) const
{
  PimaxDataType& frameData = *(PimaxDataType*) payload;
  printf( "Frame  Id 0x%05x  ReadoutTime %.2fs\n", frameData.shotIdStart(),
   frameData.readoutTime() );
}

} //namespace Pds
