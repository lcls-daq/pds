#ifndef PRINCETON_MANAGER_HH
#define PRINCETON_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pds/client/Fsm.hh"

namespace Pds 
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class PrincetonServer;

class PrincetonManager 
{
public:
  PrincetonManager(CfgClientNfs& cfg, int iCamera, bool bInitTest, int iDebugLevel);
  ~PrincetonManager();

  Appliance&    appliance() { return *_pFsm; }
  
  // Camera control: Gateway functions for accessing PrincetonServer class
  int   mapCamera(const Allocation& alloc);
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   beginRunCamera();
  int   endRunCamera();
  int   enableCamera();
  int   disableCamera();
  int   onEventReadout();
  int   getDelayData(InDatagram* in, InDatagram*& out);
  int   getLastDelayData(InDatagram* in, InDatagram*& out);
  int   checkReadoutEventCode(unsigned);
  
private:          
  const int           _iCamera;
  const bool          _bInitTest;
  const int           _iDebugLevel;
  
  Fsm*                _pFsm;
  Action*             _pActionMap;
  Action*             _pActionConfig;
  Action*             _pActionUnconfig;
  Action*             _pActionBeginRun;
  Action*             _pActionEndRun;
  Action*             _pActionEnable;  
  Action*             _pActionDisable;  
  Action*             _pActionL1Accept;
  Response*           _pResponse;
  
  PrincetonServer*    _pServer;
};

class PrincetonManagerException : public std::runtime_error
{
public:
  explicit PrincetonManagerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} // namespace Pds

#endif
