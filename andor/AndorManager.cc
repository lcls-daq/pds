#include "AndorManager.hh"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <new>
#include <vector>

#include "pds/config/AndorDataType.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "AndorServer.hh"

using std::string;

namespace Pds 
{

static int printDataTime(const InDatagram* in);

class AndorMapAction : public Action 
{
public:
    AndorMapAction(AndorManager& manager, CfgClientNfs& cfg, int iDebugLevel) : 
      _manager(manager), _cfg(cfg), _iMapCameraFail(0), _iDebugLevel(iDebugLevel) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        
        _iMapCameraFail = _manager.map(alloc.allocation());
        return tr;                
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Map Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
      if ( _iMapCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }
private:
    AndorManager& _manager;
    CfgClientNfs&     _cfg;
    int               _iMapCameraFail;    
    int               _iDebugLevel;
};

class AndorConfigAction : public Action 
{
public:
    AndorConfigAction(AndorManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _occPool(sizeof(UserMessage),2), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel),
        _cfgtc(_typeAndorConfig, cfg.src()), _config(), _iConfigCameraFail(0)
    {}
    
    virtual Transition* fire(Transition* tr) 
    {
      int iConfigSize = _cfg.fetch(*tr, _typeAndorConfig, &_config, sizeof(_config));
      
      if ( iConfigSize == 0 ) // no config data is found in the database.       
      {
        printf( "AndorConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
        _config = AndorConfigType(
          16, // Width
          16, // Height
          0,  // OrgX
          0,  // OrgX
          1,  // BinX
          1,  // BinX
          0.001,  // Exposure time
          25.0f,  // Cooling temperature
          AndorConfigType::ENUM_FAN_FULL,  // fan Mode
          0,  // baseline clamp
          0,  // high capacity
          1,  // Gain index
          1,  // Readout speed index
          1,  // Redout event code
          1   // number delay shots   
        );
      }
              
      if ( iConfigSize != 0 && iConfigSize != sizeof(_config) )
      {
        printf( "AndorConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
          iConfigSize, (int) sizeof(_config) );
          
        _config       = AndorConfigType();
        _iConfigCameraFail  = 1;
      }
                  
      string sConfigWarning;
      _iConfigCameraFail = _manager.config(_config, sConfigWarning);
      if (sConfigWarning.size() != 0)
      {
        UserMessage* msg = new(&_occPool) UserMessage();
        msg->append(sConfigWarning.c_str());
        _manager.appliance().post(msg);        
      }      
      return tr;
    }

    virtual InDatagram* fire(InDatagram* in) 
    {
        if (_iDebugLevel>=1) printf("\n\n===== Writing Configs =====\n" );
        if (_iDebugLevel>=2) printDataTime(in);

        static bool bConfigAllocated = false;        
        if ( !bConfigAllocated )
        {
          // insert assumes we have enough space in the memory pool for in datagram
          _cfgtc.alloc( sizeof(_config) );
          bConfigAllocated = true;
        }
        
        in->insert(_cfgtc, &_config);
        
        if ( _iConfigCameraFail != 0 )
          in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
                  
        if (_iDebugLevel>=2) printf( "Andor Config data:\n"
          "  Width %d Height %d  Org X %d Y %d  Bin X %d Y %d\n"
          "  Exposure time %gs  Cooling Temperature %.1f C  Gain Index %d\n"
          "  Readout Speed %d  Readout Event %d Num Delay Shots %d\n",
          _config.width(), _config.height(),
          _config.orgX(), _config.orgY(),
          _config.binX(), _config.binY(),
          _config.exposureTime(), _config.coolingTemp(), _config.gainIndex(), 
          _config.readoutSpeedIndex(), _config.exposureEventCode(), _config.numDelayShots()
          );
        if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", in->datagram().xtc.sizeofPayload());
        
        return in;
    }

private:
    AndorManager&   _manager;    
    CfgClientNfs&       _cfg;
    GenericPool         _occPool;
    bool                _bDelayMode;
    const int           _iDebugLevel;
    Xtc                 _cfgtc;
    AndorConfigType _config;    
    int                 _iConfigCameraFail;
    
    /*
     * private static consts
     */
    static const TypeId _typeAndorConfig;    
};

/*
 * Definition of private static consts
 */
const TypeId AndorConfigAction::_typeAndorConfig = TypeId(TypeId::Id_AndorConfig, AndorConfigType::Version);

class AndorUnconfigAction : public Action 
{
public:
    AndorUnconfigAction(AndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel), 
     _iUnConfigCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iUnConfigCameraFail = _manager.unconfig();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Unmap Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
      if ( _iUnConfigCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }    
private:
    AndorManager& _manager;
    int               _iDebugLevel;
    int               _iUnConfigCameraFail;
};

class AndorBeginRunAction : public Action 
{
public:
    AndorBeginRunAction(AndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iBeginRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iBeginRunCameraFail = _manager.beginRun();
      return in;
    }

    virtual InDatagram* fire(InDatagram* in) 
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing BeginRun Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
      if ( _iBeginRunCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }    
private:
    AndorManager& _manager;
    int               _iDebugLevel;
    int               _iBeginRunCameraFail;
};

class AndorEndRunAction : public Action 
{
public:
    AndorEndRunAction(AndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEndRunCameraFail = _manager.endRun();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing EndRun Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
      if ( _iEndRunCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      return in;
    }        
private:
    AndorManager& _manager;
    int               _iDebugLevel;
    int               _iEndRunCameraFail;
};

class AndorL1AcceptAction : public Action 
{
public:
    AndorL1AcceptAction(AndorManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel)
        //, _poolFrameData(1024*1024*8 + 1024, 16) // pool for debug
    {
    }
            
    virtual InDatagram* fire(InDatagram* in)     
    {               
      int         iFail = 0;
      InDatagram* out   = in;
      
/*!! recover from delay mode
      int   iShotId     = in->datagram().seq.stamp().fiducials();   // shot ID 
                        
      iFail = _manager.checkExposureEventCode(in);
      //iFail = 0; // !! debug
      
      // Discard the evr data, for appending the detector data later
      in->datagram().xtc.extent = sizeof(Xtc);            
      
      if ( iFail != 0 )
      {
        if (_iDebugLevel >= 1) 
          printf( "No readout event code\n" );
          
        return in;
      }      
      
      if ( _bDelayMode )
      {
        iFail =  _manager.getData( in, out );        
        iFail |= _manager.startExposureDelay( iShotId, in );
      }
      else
      { // prompt mode
        iFail = _manager.startExposurePrompt( iShotId, in, out );          
      }        
*/      
      bool bWait = false;
      _manager.l1Accept(bWait);
      
      if (bWait)
        iFail =  _manager.waitData( in, out );        
      else
        iFail =  _manager.getData ( in, out );        
                    
      if ( iFail != 0 )
      {
        // set damage bit
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }              
      
      /*
       * The folloing code is used for debugging variable L1 data size
       */
      //int iDataSize = 1024*1024*8 + sizeof(AndorDataType);            
      //out = 
      // new ( &_poolFrameData ) CDatagram( in->datagram() ); 
      //out->datagram().xtc.alloc( sizeof(Xtc) + iDataSize );        
      //unsigned char* pXtcHeader = (unsigned char*) out + sizeof(CDatagram);
      //   
      //TypeId typeAndorFrame(TypeId::Id_AndorFrame, AndorDataType::Version);
      //Xtc* pXtcFrame = 
      // new ((char*)pXtcHeader) Xtc(typeAndorFrame, _cfg.src() );
      //pXtcFrame->alloc( iDataSize );      

      //!!debug
      //uint32_t        uVector           = out->datagram().seq.stamp().vector();
      //printf("sending out L1 with vector %d\n", uVector);
          
      if (_iDebugLevel >= 1 && out != in) 
      {
        printf( "\n\n===== Writing L1Accept Data =========\n" );          
        if (_iDebugLevel>=2) printDataTime(in);
        
        if (_iDebugLevel >= 3) 
        {
          Xtc& xtcData = in->datagram().xtc;
          printf( "\nInput payload size = %d\n", xtcData.sizeofPayload() );
        }
        
        Xtc& xtcData = out->datagram().xtc;
        printf( "\nOutput payload size = %d  fail = %d\n", xtcData.sizeofPayload(), iFail);
        Xtc& xtcFrame = *(Xtc*) xtcData.payload();
        
        if ( xtcData.sizeofPayload() != 0 ) 
        {
          printf( "Frame  payload size = %d\n", xtcFrame.sizeofPayload());
          AndorDataType& frameData = *(AndorDataType*) xtcFrame.payload();
          printf( "Frame  Id 0x%05x  ReadoutTime %.2fs\n", frameData.shotIdStart(), 
           frameData.readoutTime() );
        }
      }

      if ( out == in && !_bDelayMode )
      {
        printf("\r");
        fflush(NULL);
        printf("\r");
        fflush(NULL);

        timeval timeSleepMicro = {0, 1000};
      // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        select( 0, NULL, NULL, NULL, &timeSleepMicro);

        //timeval timeSleepMicro = {0, 500}; // 0.5 ms
        // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        //select( 0, NULL, NULL, NULL, &timeSleepMicro);
      }
      
      return out;
    }
  
private:        
    AndorManager&   _manager;
    CfgClientNfs&       _cfg;
    bool                _bDelayMode;    
    int                 _iDebugLevel;
    //GenericPool         _poolFrameData; // pool for debug
};

class AndorBeginCalibCycleAction : public Action 
{
public:
    AndorBeginCalibCycleAction(AndorManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iBeginCalibCycleCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iBeginCalibCycleCameraFail = _manager.beginCalibCycle();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iBeginCalibCycleCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing BeginCalibCycle Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    AndorManager& _manager;
    int               _iDebugLevel;
    int               _iBeginCalibCycleCameraFail;
};

class AndorEndCalibCycleAction : public Action 
{
public:
    AndorEndCalibCycleAction(AndorManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndCalibCycleCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEndCalibCycleCameraFail = _manager.beginCalibCycle();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iEndCalibCycleCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing EndCalibCycle Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    AndorManager& _manager;
    int               _iDebugLevel;
    int               _iEndCalibCycleCameraFail;
};

class AndorEnableAction : public Action 
{
public:
    AndorEnableAction(AndorManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEnableCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEnableCameraFail = _manager.enable();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iEnableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Enable Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    AndorManager& _manager;
    int         _iDebugLevel;
    int         _iEnableCameraFail;
};

class AndorDisableAction : public Action 
{
public:
    AndorDisableAction(AndorManager& manager, int iDebugLevel) : 
     _manager(manager), _occPool(sizeof(UserMessage),2), _iDebugLevel(iDebugLevel), 
     _iDisableCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Disable Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
        
      InDatagram* out = in;
      
      // In either normal mode or delay mode, we will need to check if there is
      // an un-reported data
      int iFail = _manager.waitData( in, out );

      if ( iFail != 0 )
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit            
        
      if (_iDebugLevel >= 1) 
      {
        Xtc& xtcData = out->datagram().xtc;
        printf( "\nOutput payload size = %d  fail = %d\n", xtcData.sizeofPayload(), iFail);
        Xtc& xtcFrame = *(Xtc*) xtcData.payload();
        
        if ( xtcData.sizeofPayload() != 0 ) 
        {
          printf( "Frame  payload size = %d\n", xtcFrame.sizeofPayload());
          AndorDataType& frameData = *(AndorDataType*) xtcFrame.payload();
          printf( "Frame Id Start %d ReadoutTime %f\n", frameData.shotIdStart(), 
           frameData.readoutTime() );
        }
      }          
      
      _iDisableCameraFail = _manager.disable();
      if ( _iDisableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      
      if (out != in)
      {
        UserMessage* msg = new(&_occPool) UserMessage();
        msg->append("Run stopped before andor data is sent out,\n");
        msg->append("so the data is attached to the Disable event.");      
        _manager.appliance().post(msg);
      }
      
      return out;
    }
    
private:
    AndorManager& _manager;
    GenericPool       _occPool;
    int               _iDebugLevel;
    int               _iDisableCameraFail;
};
 
// 
//  **weaver
//
class AndorResponse : public Response {
public:
  AndorResponse(AndorManager& mgr, int iDebugLevel) :
    _manager(mgr), _iDebugLevel(iDebugLevel)
  {
  }
public:
  Occurrence* fire(Occurrence* occ) {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Get Occurance =========\n" );          
    if (_iDebugLevel>=2) printDataTime(NULL);

    const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
    if (_manager.checkExposureEventCode(cmd.code)) {
      if (_iDebugLevel >= 1) 
        printf( "Get command event code: %u\n", cmd.code );          

      _manager.startExposure();
    }
    return 0;
  }
private:
  AndorManager& _manager;
  int               _iDebugLevel;
};

AndorManager::AndorManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, 
  string sConfigDb, int iSleepInt, int iDebugLevel) :
  _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), 
  _sConfigDb(sConfigDb), _iSleepInt(iSleepInt),
  _iDebugLevel(iDebugLevel), _pServer(NULL), _uNumShotsInCycle(0)
{
  _pActionMap             = new AndorMapAction      (*this, cfg, _iDebugLevel);
  _pActionConfig          = new AndorConfigAction   (*this, cfg, _bDelayMode, _iDebugLevel);
  _pActionUnconfig        = new AndorUnconfigAction (*this, _iDebugLevel);  
  _pActionBeginRun        = new AndorBeginRunAction (*this, _iDebugLevel);
  _pActionEndRun          = new AndorEndRunAction   (*this, _iDebugLevel);  
  _pActionBeginCalibCycle = new AndorBeginCalibCycleAction 
                                                        (*this, _iDebugLevel);
  _pActionEndCalibCycle   = new AndorEndCalibCycleAction   
                                                        (*this, _iDebugLevel);  
  _pActionEnable          = new AndorEnableAction   (*this, _iDebugLevel);
  _pActionDisable         = new AndorDisableAction  (*this, _iDebugLevel);
  _pActionL1Accept        = new AndorL1AcceptAction (*this, cfg, _bDelayMode, _iDebugLevel);
  _pResponse              = new AndorResponse       (*this, _iDebugLevel);

  try
  {     
  _pServer = new AndorServer(_iCamera, _bDelayMode, _bInitTest, cfg.src(), _sConfigDb, _iSleepInt, _iDebugLevel);    
  }
  catch ( AndorServerException& eServer )
  {
    throw AndorManagerException( "AndorManager::AndorManager(): Server Initialization Failed" );
  }

  _pFsm = new Fsm();    
  _pFsm->callback(TransitionId::Map,              _pActionMap);
  _pFsm->callback(TransitionId::Configure,        _pActionConfig);
  _pFsm->callback(TransitionId::Unconfigure,      _pActionUnconfig);
  _pFsm->callback(TransitionId::BeginRun,         _pActionBeginRun);
  _pFsm->callback(TransitionId::EndRun,           _pActionEndRun);
  _pFsm->callback(TransitionId::BeginCalibCycle,  _pActionBeginCalibCycle);
  _pFsm->callback(TransitionId::EndCalibCycle,    _pActionEndCalibCycle);    
  _pFsm->callback(TransitionId::Enable,           _pActionEnable);            
  _pFsm->callback(TransitionId::Disable,          _pActionDisable);            
  _pFsm->callback(TransitionId::L1Accept,         _pActionL1Accept);
  _pFsm->callback(OccurrenceId::EvrCommand,       _pResponse);
}

AndorManager::~AndorManager()
{   
  delete _pFsm;
  
  delete _pServer;
  
  delete _pResponse;
  delete _pActionL1Accept;
  delete _pActionDisable;
  delete _pActionEnable;
  delete _pActionEndRun;
  delete _pActionBeginRun;
  delete _pActionUnconfig;    
  delete _pActionConfig;
  delete _pActionMap; 
}

int AndorManager::map(const Allocation& alloc)
{
  return _pServer->map();
}

int AndorManager::config(AndorConfigType& config, std::string& sConfigWarning)
{
  if ( !_bDelayMode )
    config.setNumDelayShots(0);
  
  return _pServer->config(config, sConfigWarning);
}

int AndorManager::unconfig()
{
  return _pServer->unconfig();
}

int AndorManager::beginRun()
{
  return _pServer->beginRun();
}

int AndorManager::endRun()
{
  return _pServer->endRun();
}

int AndorManager::beginCalibCycle()
{
  _uNumShotsInCycle = 0;
  return _pServer->beginCalibCycle();
}

int AndorManager::endCalibCycle()
{
  return _pServer->endCalibCycle();
}

int AndorManager::enable()
{
  return _pServer->enable();
}

int AndorManager::disable()
{
  return _pServer->disable();
}

int AndorManager::l1Accept(bool& bWait)
{ 
  ++_uNumShotsInCycle;
  
  if (!_bDelayMode)
    bWait = false;
  else
    bWait = (_uNumShotsInCycle>= _pServer->config().numDelayShots());
  return 0;
}

int AndorManager::startExposure()
{
  return _pServer->startExposure();
}

/* !! recover from delay mode
int AndorManager::startExposurePrompt(int iShotId, InDatagram* in, InDatagram*& out)
{
  return _pServer->startExposurePrompt(iShotId, in, out);  
}

int AndorManager::startExposureDelay(int iShotId, InDatagram* in)
{
  return _pServer->startExposureDelay(iShotId, in);  
}
*/

int AndorManager::getData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getData(in, out);
}

int AndorManager::waitData(InDatagram* in, InDatagram*& out)
{
  int iFail = _pServer->waitData(in, out);
  
  if (out != in)
    _uNumShotsInCycle = 0;
    
  return iFail;
}

int AndorManager::checkExposureEventCode(unsigned code)
{
  return (code == _pServer->config().exposureEventCode());
}

/* 
 * Print the local timestamp and the data timestamp
 *
 * If the input parameter in == NULL, no data timestamp will be printed
 */
static int printDataTime(const InDatagram* in)
{
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */    
  char sTimeText[40];
  
  time_t timeCurrent;
  time(&timeCurrent);          
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeCurrent));
  
  printf("Local Time: %s\n", sTimeText);

  if ( in == NULL ) return 0;
  
  const ClockTime clockCurDatagram  = in->datagram().seq.clock();
  uint32_t        uFiducial         = in->datagram().seq.stamp().fiducials();
  uint32_t        uVector           = in->datagram().seq.stamp().vector();
  timeCurrent                       = clockCurDatagram.seconds();
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeCurrent));
  printf("Data  Time: %s  Fiducial: 0x%05x Vector: %d\n", sTimeText, uFiducial, uVector);
  return 0;
}

} //namespace Pds 