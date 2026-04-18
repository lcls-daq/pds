#include "FrameBuffer.hh"
#include "Driver.hh"
#include "Server.hh"
#include "Errors.hh"

#include "VmbImageTransform/VmbTransform.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace Pds::Vimba;

FrameBuffer::FrameBuffer(size_t nbuffers, Camera* cam):
  _cam(cam),
  _nbuffers(nbuffers),
  _frames(new VmbFrame_t[nbuffers]),
  _buffer(NULL),
  _buffer_sz(0)
{}

FrameBuffer::~FrameBuffer()
{
  if (_frames) {
    delete[] _frames;
  }
  if (_buffer) {
    delete[] _buffer;
  }
}

void FrameBuffer::configure()
{
  if (_cam) {
    VmbUint32_t payloadSize = _cam->payloadSize();

    // allocate memory for buffers if needed
    if (_buffer_sz < _nbuffers * payloadSize) {
      if (_buffer) delete[] _buffer;
      _buffer = new char[_nbuffers * payloadSize];
      _buffer_sz = _nbuffers * payloadSize;
    }

    // initialize the frames with buffer info
    for (size_t n=0; n<_nbuffers; n++) {
      _frames[n].buffer = _buffer + (payloadSize * n);
      _frames[n].bufferSize = payloadSize;
      _frames[n].context[0] = this;
    }

    // register allocated frames
    _cam->registerFrames(_frames, _nbuffers);
    // start capture engine
    _cam->captureStart();
    // queue frames
    _cam->queueFrames(_frames, _nbuffers, frameCallBack);
  }
}

void FrameBuffer::unconfigure()
{
  if (_cam) {
    // end capture
    _cam->captureEnd();
    // flush the frame queue
    _cam->flushFrames();
    // unregister the frames
    _cam->unregisterAllFrames();
  }
}

bool FrameBuffer::enable()
{
  if (_cam) {
    return _cam->acquisitionStart();
  } else {
    return false;
  }
}

bool FrameBuffer::disable()
{
  if (_cam) {
    return _cam->acquisitionStop();
  } else {
    return false;
  }
}

VmbUint32_t FrameBuffer::sizeAs16Bit(VmbFrame_t* frame)
{
  return frame->width * frame->height * sizeof(VmbMono16_t);
}

bool FrameBuffer::copyAs16Bit(VmbFrame_t* frame, void* buffer)
{
  switch (frame->pixelFormat) {
    case VmbPixelFormatMono8:
    case VmbPixelFormatMono10:
    case VmbPixelFormatMono12:
    case VmbPixelFormatMono14:
    case VmbPixelFormatMono16:
      return copyAs16BitStd(frame, buffer);
    case VmbPixelFormatMono10p:
    case VmbPixelFormatMono12Packed:
    case VmbPixelFormatMono12p:
      return copyAs16BitVmb(frame, buffer);
    default:
      fprintf(stderr, "%s error: frame has an unsupported pixel encoding - %s\n", __FUNCTION__, PixelFormatTypes::desc(frame->pixelFormat));
      return false;
  }
  return true;
}

bool FrameBuffer::copyAs16BitVmb(VmbFrame_t* frame, void* buffer)
{
  VmbError_t err = VmbErrorSuccess;
  VmbImage src;
  VmbImage dest;

  // set size member for verification inside API
  src.Size = sizeof(src);
  src.Data = frame->buffer;
  dest.Size = sizeof(dest);
  dest.Data = buffer;

  err = VmbSetImageInfoFromPixelFormat(frame->pixelFormat, frame->width, frame->height, &src);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to set input image info - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  }

  err = VmbSetImageInfoFromInputImage(&src, VmbPixelLayoutMono, 16, &dest);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to set output image info - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  }

  err = VmbImageTransform (&src, &dest, NULL, 0);
  if (err != VmbErrorSuccess) {
    fprintf(stderr, "%s error: failed to transform image - %s\n", __FUNCTION__, ErrorCodes::desc(err));
    return false;
  } else {
    return true;
  }
}

bool FrameBuffer::copyAs16BitStd(VmbFrame_t* frame, void* buffer)
{
  char* frame_start = (char*) frame->buffer;
  char* frame_end = frame_start + frame->bufferSize;
  if (frame->pixelFormat & VmbPixelOccupy16Bit) {
    std::copy((uint16_t*) frame_start, (uint16_t*) frame_end, (uint16_t*) buffer);
    return true;
  } else if (frame->pixelFormat & VmbPixelOccupy8Bit) {
    std::copy((uint8_t*) frame_start, (uint8_t*) frame_end, (uint16_t*) buffer);
    return true;
  } else {
    fprintf(stderr, "%s error: frame has an unsupported pixel encoding - %s\n", __FUNCTION__, PixelFormatTypes::desc(frame->pixelFormat));
    return false;
  }
}

void VMB_CALL FrameBuffer::frameCallBack(const VmbHandle_t hcam, const VmbHandle_t hstream, VmbFrame_t* ptr)
{
  FrameBuffer* framebuf = reinterpret_cast<FrameBuffer*>(ptr->context[0]);
  // process the frame
  framebuf->process(ptr);
  // requeue frame
  VmbCaptureFrameQueue(hcam, ptr, frameCallBack);
}

SimpleFrameBuffer::SimpleFrameBuffer(size_t nbuffers, size_t nframes, Camera* cam, const char* file_prefix, bool show_stats, bool reformat_pixels):
  FrameBuffer(nbuffers, cam),
  _nframes(nframes),
  _ncomp(0),
  _file_prefix(file_prefix),
  _show_stats(show_stats),
  _reformat_pixels(reformat_pixels),
  _sem(Semaphore::FULL),
  _fname(file_prefix ? new char[std::strlen(file_prefix) + 15] : NULL)
{}

SimpleFrameBuffer::~SimpleFrameBuffer()
{
  if (_fname) {
    delete[] _fname;
  }
}

bool SimpleFrameBuffer::enable()
{
  _ncomp = 0;
  bool started = FrameBuffer::enable();
  if (started) {
    _sem.take();
  }
  return started;
}

bool SimpleFrameBuffer::disable()
{
  bool stopped = FrameBuffer::disable();
  if (stopped) {
    _sem.give();
  }
  return stopped;
}

void SimpleFrameBuffer::wait()
{
  _sem.take();
}

void SimpleFrameBuffer::cancel()
{
  _sem.give();
}

void SimpleFrameBuffer::process(VmbFrame_t* frame)
{
  // ignore extra frames on fixed acquisition
  if (_nframes > 0 && _ncomp >= _nframes) return;

  if (frame->receiveStatus == VmbFrameStatusComplete) {
    if (_nframes == 0) {
      printf("\033[32mRecieved frame %zu\033[0m\n", _ncomp+1);
    } else {
      printf("\033[32mRecieved frame %zu of %zu\033[0m\n", _ncomp+1, _nframes);
    }
  } else {
    if (_nframes == 0) {
      printf("\033[31mError recieving frame %zu: %s\033[0m\n",
             _ncomp+1, FrameStatusCodes::desc(frame->receiveStatus));
    } else {
      printf("\033[31mError recieving frame %zu of %zu: %s\033[0m\n",
             _ncomp+1, _nframes, FrameStatusCodes::desc(frame->receiveStatus));
    }
  }
  if (_show_stats) {
    printf("timestamp: %llu\n", frame->timestamp);
    printf("status: %s\n", FrameStatusCodes::desc(frame->receiveStatus));
    printf("framed id %llu\n", frame->frameID);
    printf("format %s\n", PixelFormatTypes::desc(frame->pixelFormat));
  }
  _ncomp++;

  // write images to file if requested
  if (_file_prefix && _fname) {
    sprintf(_fname, "%s%04zu.raw", _file_prefix, _ncomp);
    FILE *img = fopen(_fname, "wb");
    if (_reformat_pixels) {
      VmbUint32_t convertedImageSize = FrameBuffer::sizeAs16Bit(frame);
      char* convertedImage = new char[convertedImageSize];
      FrameBuffer::copyAs16Bit(frame, convertedImage);
      fwrite(convertedImage, 1, convertedImageSize, img);
      delete[] convertedImage;
    } else {
      fwrite(frame->buffer, 1, frame->bufferSize, img);
    }
    fclose(img);
  }

  // single that acquistion is done
  if (_ncomp == _nframes) {
    _sem.give();
  }
}

ServerFrameBuffer::ServerFrameBuffer(size_t nbuffers, Camera* cam, Server* srv) :
  FrameBuffer(nbuffers, cam),
  _srv(srv)
{}

ServerFrameBuffer::~ServerFrameBuffer()
{}

void ServerFrameBuffer::configure()
{
  // call the base class implementation
  FrameBuffer::configure();
  if (_cam) {
    // show readout buffer information
    printf("Buffer information:\n");
    printf("  Driver Buffer Count:    %lld\n", _cam->maxDriverBuffersCount());
    printf("  Stream Buffer Mode:     %s\n",   _cam->streamBufferHandlingMode());
    printf("  Stream Buffer Minimum:  %lld\n", _cam->streamAnnounceBufferMinimum());
    printf("  Stream Buffer Count:    %lld\n", _cam->streamAnnouncedBufferCount());
  }
}

void ServerFrameBuffer::process(VmbFrame_t* frame)
{
  _srv->post(frame);
}
