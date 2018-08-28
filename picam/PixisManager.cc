#include "pds/picam/PixisManager.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pds/config/PixisConfigType.hh"
#include "pds/config/PixisDataType.hh"
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

void PixisManager::printConfigInfo(PicamConfig* config) const
{
  PixisConfigWrapper* pixis_config = dynamic_cast<PixisConfigWrapper*>(config);
  if (pixis_config) {
    printf( "Pixis Config data:\n"
      "  Width %d Height %d  Org X %d Y %d  Bin X %d Y %d\n"
      "  Cooling Temperature %.1f C  Readout Speed %g MHz  Gain Index %d\n"
      "  ADC Mode %d  Trigger %d  Vertical Shift Speed %g ns\n"
      "  InfoRepInt %d  Readout Event %d  Num Integration Shots %d\n",
      pixis_config->width(), pixis_config->height(),
      pixis_config->orgX(), pixis_config->orgY(),
      pixis_config->binX(), pixis_config->binY(),
      pixis_config->coolingTemp(), pixis_config->readoutSpeed(), pixis_config->gainIndex(),
      pixis_config->adcMode(), pixis_config->triggerMode(), pixis_config->vsSpeed(),
      pixis_config->infoReportInterval(), pixis_config->exposureEventCode(), pixis_config->numIntegrationShots()
    );
  }
}

void PixisManager::printFrameInfo(void* payload) const
{
  PixisDataType& frameData = *(PixisDataType*) payload;
  printf( "Frame  Id 0x%05x  ReadoutTime %.2fs\n", frameData.shotIdStart(),
   frameData.readoutTime() );
}

} //namespace Pds
