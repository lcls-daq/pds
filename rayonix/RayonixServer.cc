#include "RayonixServer.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/camera/FrameType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;


#define MAX_LINE_PIXELS     3840
#define MAX_FRAME_PIXELS    (MAX_LINE_PIXELS * MAX_LINE_PIXELS)
#define RAYONIX_DEPTH       16
#define PAYLOAD_MAX         10000


Pds::RayonixServer::RayonixServer( const Src& client, bool verbose)
  : _xtc        ( _frameType, client ),
    _xtcDamaged ( _frameType, client ),
    _occSend(NULL),
    _verbose(verbose),
    _binning_f(0),
    _binning_s(0),
    _darkFlag(0),
    _readoutMode(0),
    _trigger(0),
    _rnxctrl(NULL)
{
  // Calculate xtc extent
  _xtc.extent = sizeof(Pds::Camera::FrameV1) + sizeof(Xtc);
  
  // xtc extent in case of damaged event
  _xtcDamaged.extent = sizeof(Xtc);
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

}


unsigned Pds::RayonixServer::configure(RayonixConfigType& config)
{
  unsigned numErrs = 1;
  int status;
  int binning_f = (int)config.binning_f();
  int binning_s = (int)config.binning_s();
  int exposure = (int)config.exposure();
  int trigger = (int)config.trigger();
  int rawMode = (int)config.rawMode();
  int darkFlag = (int)config.darkFlag();
  int readoutMode = (int)config.readoutMode();    
  char deviceBuf[Pds::rayonix_control::DeviceIDMax+1];
  // FIX this - for now, testPattern is 0
  int testPattern = 0;
  char msgBuf[100];

  if (verbose() > 0) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_rnxctrl) {
    printf("ERROR: _rnxctrl is not NULL at beginning of %s\n", __PRETTY_FUNCTION__);
  }

  // If the binning is not the same in both fast and slow directions, send an occurrence
  if (_binning_f != _binning_s) {
    snprintf(msgBuf, sizeof(msgBuf), "ERROR: binning_f is not equal to binning_s; Rayonix is not calibrated for this\n");
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      _occSend->userMessage(msgBuf);
    }
  }

  _count = 0;

  // create rayonix control object (to be deleted in unconfigure())
  _rnxctrl = new rayonix_control((verbose() > 0), RNX_IPADDR, RNX_CONTROL_PORT);
  printf("Created rayonix control object with IP addr = %s, Port = %d\n", RNX_IPADDR, RNX_CONTROL_PORT);

  if (_rnxctrl) {
    do {
      printf("Calling _rnxctrl->connect()...\n");
      _rnxctrl->connect();
      status = _rnxctrl->status();
      if (status == Pds::rayonix_control::Unconfigured) {
         
        if (_rnxctrl->config(binning_f, binning_s, exposure, rawMode, readoutMode, trigger,
                             testPattern, darkFlag, deviceBuf)) {
          printf("ERROR: _rnxctrl->config() failed\n");
          break;    /* ERROR */
        } else {
          _binning_f = binning_f;
          _binning_s = binning_s;
          _exposure  = exposure;
          _darkFlag  = darkFlag;
          _readoutMode = readoutMode;
          _trigger   = trigger;
          if (deviceBuf) {
            // copy to _deviceID
            strncpy(_deviceID, deviceBuf, Pds::rayonix_control::DeviceIDMax);
            printf("Rayonix Device ID: \"%s\"\n", _deviceID);
          }
        }
        printf("Calling _rnxctrl->enable()...\n");
        if (_rnxctrl->enable()) {
          printf("ERROR: _rnxctrl->enable() failed\n");
          break;    /* ERROR */
        }
        numErrs = 0;  /* Success */
      } else {
        printf("ERROR: _rnxctrl->connect() failed (status==%d) in %s\n", status, __PRETTY_FUNCTION__);
      }
    } while (0);
  } else {
    printf("ERROR: _rnxctrl is NULL in %s\n", __PRETTY_FUNCTION__);
  }

  return (numErrs);
}

unsigned Pds::RayonixServer::unconfigure(void)
{
  unsigned numErrs = 0;

  if (verbose() > 0) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_rnxctrl) {
    _rnxctrl->disable();
    _rnxctrl->reset();
    _rnxctrl->disconnect();
    if (_rnxctrl->status() == Pds::rayonix_control::Unconnected) {
      delete _rnxctrl;
      _rnxctrl = NULL;
    } else {
      printf("ERROR: _rnxctrl->status() != Unconnected in %s\n", __PRETTY_FUNCTION__);
      ++ numErrs;
    }
  } else {
    printf("Warning: _rnxctrl is NULL in %s\n", __PRETTY_FUNCTION__);
  }

  return (numErrs);
}


unsigned Pds::RayonixServer::endrun(void)
{
  printf("Rayonix endrun called\n");
  return(0);
}


int Pds::RayonixServer::fetch( char* payload, int flags )
{
  uint16_t frameNumber;
  // Fix this - where do we define the width, height, and depth in bytes?
  int width = MAX_LINE_PIXELS/Pds::RayonixServer::_binning_f;
  int height =MAX_LINE_PIXELS/Pds::RayonixServer::_binning_s;
  int depth = RAYONIX_DEPTH;
  int offset = 0;

  Pds::rayonix_data * rnxdata = new Pds::rayonix_data::rayonix_data(MAX_FRAME_PIXELS, RayonixServer::verbose());

  printf(" *** entered %s\n", __PRETTY_FUNCTION__);

  if (_outOfOrder) {
    return(-1);
  }

    
  // copy xtc to payload
  memcpy(payload, &_xtc, sizeof(Xtc));
  offset += sizeof(Xtc);

  //  Pds::Camera::FrameV1::FrameV1 *frame = (Pds::Camera::FrameV1::FrameV1 *)(payload + sizeof(Xtc));

  *new(payload+offset) Pds::Camera::FrameV1::FrameV1(width, height, depth, 0);

  rnxdata->readFrame(frameNumber, (payload+offset), PAYLOAD_MAX, RayonixServer::_binning_f, 
            RayonixServer::_binning_s, RayonixServer::verbose());
  if (RayonixServer::verbose()) {
    printf("%s: received frame #%u\n", __PRETTY_FUNCTION__, frameNumber);
  }
    // if error, damaged
    //    memcpy(payload, &_xtcDamaged, sizeof(Xtc));
    

  return (_xtc.extent);
}
                               

unsigned Pds::RayonixServer::count() const
{
  return(_count-1);
}

bool Pds::RayonixServer::verbose() const
{
  return(_verbose);
}

void Pds::RayonixServer::setOccSend(RayonixOccurrence* occSend)
{
  _occSend = occSend;
}
