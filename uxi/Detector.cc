#include "pds/uxi/Detector.hh"

#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define REPLY_SIZE 64
#define CMD_SIZE 32

#pragma pack(push)
#pragma pack(4)
struct uxi_header {
  uint32_t frame;
  uint32_t height;
  uint32_t width;
  uint32_t number;
  uint32_t timestamp;
  double temperature;
};

struct uxi_timing {
  char side;
  uint32_t ton;
  uint32_t toff;
  uint32_t tdel;
};

struct uxi_pots {
  double voltage;
  uint32_t tune;
};
#pragma pack(pop)

using namespace Pds::Uxi;

Detector::Detector(const char* host, unsigned comm_port, unsigned data_port) :
  _host(host),
  _data_port(data_port),
  _comm_port(comm_port),
  _data_up(false),
  _comm_up(false),
  _header_buf(new char[sizeof(uxi_header)]),
  _context(0),
  _data(0)
{
  _context = zmq_ctx_new();
  connect();
}

Detector::~Detector()
{
  disconnect();
  if (_context) {
    zmq_ctx_destroy(_context);
    _context = 0;
  }
  if (_header_buf) {
    delete[] _header_buf;
  }
}

void Detector::connect()
{
  if (!_data_up) {
    _data = zmq_socket(_context, ZMQ_SUB);
    int rc = zmq_setsockopt(_data, ZMQ_SUBSCRIBE, "", 0 );
    if (rc < 0) {
      fprintf(stderr, "Error: failed to set ZMQ_SUBSCRIBE sockopt: %s\n", strerror(errno));
    } else {
      _data_up = connect_socket(_data, _data_port);
    }
  }

  if (!_comm_up) {
    _comm = zmq_socket(_context, ZMQ_REQ);
    _comm_up = connect_socket(_comm, _comm_port);
  }
}

void Detector::disconnect()
{
  if (_data) {
    _data_up = false;
    zmq_close(_data);
    _data = 0;
  }
  if (_comm) {
    _comm_up = false;
    zmq_close(_comm);
    _comm = 0;
  }
}

bool Detector::connected() const
{
  return _data_up && _comm_up;
}

bool Detector::connect_socket(void* sock, unsigned port)
{
  int rc;
  char endpoint[strlen(_host) + 30];

  snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", _host, port);
  rc = zmq_connect(sock, endpoint);
  if (rc<0) {
    fprintf(stderr, "Error: failed to connect to Uxi at %s on port %d: %s\n", _host, port, strerror(errno));
    return false;
  } else {
    return true;
  }
}

bool Detector::get_frames(uint32_t& acq_num, uint16_t* data, double* temp, uint32_t* timestamp)
{
  if (!_data_up) {
    fprintf(stderr, "Error: cannot get frames when the data link to uxi is down!\n");
    return false;
  }

  int rc = zmq_recv(_data, _header_buf, sizeof(uxi_header), 0);
  if (rc<0) {
    fprintf(stderr, "Error: failure on recv of data header: %s\n", strerror(errno));
    return false;
  } else if (rc!=sizeof(uxi_header)) {
    fprintf(stderr, "Error: data header size %d differs from expected %lu\n", rc, sizeof(uxi_header));
    return false;
  } else {
    struct uxi_header* hdr = (struct uxi_header*) _header_buf;
    acq_num = hdr->frame;
    if (temp) {
      *temp = hdr->temperature;
    }
    if (timestamp) {
      *timestamp = hdr->timestamp;
    }

    bool status = true;
    for (unsigned i=0; i<hdr->number; i++) {
      rc = get_frame_part(data, hdr->height*hdr->width*sizeof(uint16_t));
      
      if (rc <= 0) {
        fprintf(stderr, "Error: failed to read frame %d payload", i);
        status = false;
        break;
      } else if(((unsigned) rc) != hdr->height*hdr->width*sizeof(uint16_t)) {
        fprintf(stderr, "Error: frame %d payload size %d differs from expected %lu\n", i, rc, hdr->height*hdr->width*sizeof(uint16_t));
        status = false;
      }
      data += hdr->height*hdr->width;
    }

    return status;
  }
}

int Detector::get_frame_part(uint16_t* data, size_t len)
{
  int rc;
  int64_t more;
  size_t more_size = sizeof more;

  rc = zmq_getsockopt(_data, ZMQ_RCVMORE, &more, &more_size);

  if (rc < 0) {
    fprintf(stderr, "Error: failed to get ZMQ_RCVMORE sockopt: %s\n", strerror(errno));
    return rc;
  } else if(more) {
    rc = zmq_recv(_data, data, len, 0);
    return rc;
  } else {
    return 0;
  }
}

bool Detector::temperature(double* temp)
{
  return get_double("TEMP?", temp);
}

bool Detector::width(uint32_t* width)
{
  return get_uint32("WIDTH?", width);
}

bool Detector::height(uint32_t* height)
{
  return get_uint32("HEIGHT?", height);
}

bool Detector::nframes(uint32_t* nframes)
{
  return get_uint32("NFRAMES?", nframes);
}

bool Detector::nbytes(uint32_t* nbytes)
{
  return get_uint32("NBYTES?", nbytes);
}

bool Detector::type(uint32_t* type)
{
  return get_uint32("TYPE?", type);
}

bool Detector::acq_count(uint32_t* acq_count)
{
  return get_uint32("ACQCNT?", acq_count);
}

bool Detector::num_pixels(uint32_t* num_pixels)
{
  uint32_t width_rbv, height_rbv, nframes_rbv;
  if (width(&width_rbv) && height(&height_rbv) && nframes(&nframes_rbv)) {
    *num_pixels = width_rbv * height_rbv * nframes_rbv;
    return true;
  } else {
    return false;
  }
}

bool Detector::frame_size(uint32_t* frame_size)
{
  uint32_t width_rbv, height_rbv, nframes_rbv, nbytes_rbv;
  if (width(&width_rbv) && height(&height_rbv) &&
      nframes(&nframes_rbv) && nbytes(&nbytes_rbv)) {
    *frame_size = width_rbv * height_rbv * nframes_rbv * nbytes_rbv;
    return true;
  } else {
    return false;
  }
}

bool Detector::num_pots(uint32_t* num_pots)
{
  return get_uint32("NPOTS?", num_pots);
}

bool Detector::get_mon(unsigned pot, double* value)
{
  char cmd[CMD_SIZE];
  snprintf(cmd, sizeof(cmd), "MON%d?", pot);
  return get_double(cmd, value);
}

bool Detector::get_pot(unsigned pot, double* value)
{
  char cmd[CMD_SIZE];
  snprintf(cmd, sizeof(cmd), "POT%d?", pot);
  return get_double(cmd, value);
}

bool Detector::set_pot(unsigned pot, double value, bool tune)
{
  struct uxi_pots pot_value = { value, tune ? 1u : 0u };
  char cmd[CMD_SIZE];
  snprintf(cmd, sizeof(cmd), "POT%d", pot);
  return put_command(cmd, &pot_value, sizeof(pot_value));
}

bool Detector::get_timing(char side, unsigned* ton, unsigned* toff, unsigned* delay)
{
  struct uxi_timing value;
  char cmd[CMD_SIZE];
  snprintf(cmd, sizeof(cmd), "TIMING%c?", side);
  if (get_command(cmd, &value, sizeof(value))) {
    *ton = value.ton;
    *toff = value.toff;
    if (delay)
      *delay = value.tdel;
    return true;
  } else {
    return false;
  }
}

bool Detector::set_timing_all(unsigned ton, unsigned toff, unsigned delay)
{
  return set_timing('A', ton, toff, delay) && set_timing('B', ton, toff, delay);
}

bool Detector::set_timing(char side, unsigned ton, unsigned toff, unsigned delay)
{
  struct uxi_timing value = { side, ton, toff, delay };
  char cmd[CMD_SIZE];
  snprintf(cmd, sizeof(cmd), "TIMING%c", side);
  return put_command(cmd, &value, sizeof(value));
}

bool Detector::get_uint32(const char* cmd, uint32_t* value)
{
  if (value) {
    return get_command(cmd, value, sizeof(uint32_t));
  } else {
    return false;
  }
}

bool Detector::get_double(const char* cmd, double* value)
{
  if (value) {
    return get_command(cmd, value, sizeof(double));
  } else {
    return false;
  }
}


bool Detector::acquire(unsigned nframes)
{
  return put_command("START", &nframes, sizeof(nframes));
}

bool Detector::start()
{
  return acquire();
}

bool Detector::stop()
{
  return put_command("STOP");
}

bool Detector::commit()
{
  return put_command("CONFIG");
}

bool Detector::put_command(const char* cmd, void* payload, size_t size)
{
  char reply[REPLY_SIZE];
  memset(reply, 0, sizeof reply);
  if (!_comm_up) {
    fprintf(stderr, "Error: cannot send command %s when comm link to uxi is down!\n", cmd);
    return false;
  }

  int rc = zmq_send(_comm, cmd, strlen(cmd), payload ? ZMQ_SNDMORE : 0);
  if (rc < 0) {
    fprintf(stderr, "Error: sending command %s to uxi: %s\n", cmd, strerror(errno));
    return false;
  }
  if (payload) {
    rc = zmq_send(_comm, payload, size, 0);
    if (rc < 0) {
      fprintf(stderr, "Error: sending payload of command %s to uxi: %s\n", cmd, strerror(errno));
      return false;
    }
  }

  rc = zmq_recv(_comm, reply, sizeof(reply), 0);
  if (rc < 0) {
    fprintf(stderr, "Error: receiving reply to command %s: %s\n", cmd, strerror(errno));
    return false;
  } else if (strcmp(reply, "OK") != 0) {
    fprintf(stderr, "Error: uxi returned an error in reply to command %s: %s\n", cmd, reply);
    return false;
  } else {
    return true;
  }
}

bool Detector::get_command(const char* cmd, void* payload, size_t size)
{
  char reply[REPLY_SIZE];
  memset(reply, 0, sizeof reply);
  if (!_comm_up) {
    fprintf(stderr, "Error: cannot send command %s when the comm link to uxi is down!\n", cmd);
    return false;
  }

  int rc = zmq_send(_comm, cmd, strlen(cmd), 0);
  if (rc < 0) {
    fprintf(stderr, "Error: sending command %s to uxi: %s\n", cmd, strerror(errno));
    return false;
  } else {
    rc = zmq_recv(_comm, reply, sizeof(reply), 0);
    if (rc < 0) {
      fprintf(stderr, "Error: receiving reply to command %s: %s\n", cmd, strerror(errno));
      return false;
    } else if (strcmp(reply, "OK") != 0) {
      fprintf(stderr, "Error: uxi returned an error in reply to command %s: %s\n", cmd, reply);
      return false;
    } else {
      rc = zmq_recv(_comm, payload, size, 0);
      if (rc < 0) {
        fprintf(stderr, "Error: receiving reply to command %s: %s\n", cmd, strerror(errno));
        return false;
      } else if (((unsigned) rc) != size) {
        fprintf(stderr, "Error: command %s - reply size %d differs from expected %lu\n", cmd, rc, size);
        return false;
      } else {
        return true;
      }
    }
  }
}

#undef REPLY_SIZE
#undef CMD_SIZE
