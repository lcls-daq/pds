#ifndef PIXIS_MANAGER_HH
#define PIXIS_MANAGER_HH

#include "pds/picam/PicamManager.hh"

namespace Pds
{

class CfgClientNfs;

class PixisManager : public PicamManager
{
public:
  PixisManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel);
  ~PixisManager();

  void printConfigInfo(PicamConfig* config) const;
  void printFrameInfo(void* payload) const;
};

} // namespace Pds

#endif
