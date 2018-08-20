#ifndef PIMAX_MANAGER_HH
#define PIMAX_MANAGER_HH

#include "pds/picam/PicamManager.hh"

namespace Pds
{

class CfgClientNfs;

class PimaxManager : public PicamManager
{
public:
  PimaxManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel);
  ~PimaxManager();
};

} // namespace Pds

#endif
