#include "Sensor.hh"
#include "Daedalus.hh"
#include "Icarus.hh"
#include "Icarus2.hh"
#include "Error.hh"
#include "Logger.hh"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

static constexpr uint64_t timing_bits = UINT64_C(40);
static constexpr uint64_t timing_bitmask = (UINT64_C(1)<<timing_bits) - 1;
static constexpr size_t manual_seq_len = 7;
static constexpr uint64_t manual_seq_scale = UINT64_C(25);
static constexpr uint64_t min_manual_timing = UINT64_C(0x4b);
static constexpr uint64_t max_manual_timing = UINT64_C(0x640000000);

std::ostream& Pds::NsCam::operator<<(std::ostream& os, const Timing& timing)
{
  os << "[";
  os << " open = " << timing.open << ",";
  os << " closed = " << timing.closed << ",";
  os << " delay = " << timing.delay << " ]";
  return os;
}

std::ostream& Pds::NsCam::operator<<(std::ostream& os, const Sequence& sequence)
{
  bool first = true;
  os << "[";
  for (const auto& seq : sequence) {
    if (first) {
      os << " " << seq;
      first = false;
    } else {
      os << ", " << seq;
    }
  }
  os << " ]";
  return os;
}

std::string Pds::NsCam::toString(const Sequence& sequence)
{
  bool first = true;
  std::ostringstream seqstr;
  seqstr << "[";
  for (const auto& seq : sequence) {
    if (first) {
      seqstr << " " << seq;
      first = false;
    } else {
      seqstr << ", " << seq;
    }
  }
  seqstr << " ]";
  return seqstr.str();
}

using namespace Pds::NsCam;

std::string Timing::toString(const Timing& timing)
{
  std::ostringstream timingstr;
  timingstr << "[";
  timingstr << " open = " << timing.open << ",";
  timingstr << " closed = " << timing.closed << ",";
  timingstr << " delay = " << timing.delay << " ]";

  return timingstr.str();
}

std::shared_ptr<Sensor> Sensor::create(SensorType stype, std::shared_ptr<Board> board)
{
  switch (stype) {
  case SensorType::DAEDALUS:
    return std::make_shared<Daedalus>(board);
  case SensorType::ICARUS:
    return std::make_shared<Icarus>(board);
  case SensorType::ICARUS2:
    return std::make_shared<Icarus2>(board);
  default:
    return std::shared_ptr<Sensor>{};
  }
}

Sensor::Sensor(SensorType stype,
               std::shared_ptr<Board> board,
               uint32_t nseq,
               uint32_t minframe,
               uint32_t maxframe,
               uint32_t maxwidth,
               uint32_t maxheight,
               uint32_t bytesperpixel) :
  stype_(stype),
  board_(board),
  nseq_(nseq),
  minframe_(minframe),
  maxframe_(maxframe),
  maxwidth_(maxwidth),
  maxheight_(maxheight),
  firstframe_(minframe),
  lastframe_(maxframe),
  firstrow_(0),
  lastrow_(maxheight - 1),
  bytesperpixel_(bytesperpixel),
  interlacing_{0, 0},
  manualTiming_(false)
{}

void Sensor::info() const
{
  // save the i/o formatting before changing...
  FormatBackup fmt(std::cout);
  std::cout << "Sensor Info:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Type:             " << name() << std::endl;
  std::cout << " Min Frame:        " << minframe_ << std::endl;
  std::cout << " Max Frame:        " << maxframe_ << std::endl;
  std::cout << " Max Width:        " << maxwidth_ << std::endl;
  std::cout << " Max Height:       " << maxheight_ << std::endl;
  std::cout << " Bytes Per Pixel:  " << bytesperpixel_ << std::endl;
  std::cout << " First Frame:      " << firstframe_ << std::endl;
  std::cout << " Last Frame:       " << lastframe_ << std::endl;
  std::cout << " Num Frames:       " << nframes() << std::endl;
  std::cout << " First Row:        " << firstrow_ << std::endl;
  std::cout << " Last Row:         " << lastrow_ << std::endl;
  std::cout << " Width:            " << width() << std::endl;
  std::cout << " Height:           " << height() << std::endl;
  std::cout << " Num Pixels:       " << npixels() << std::endl;
  std::cout << " Frame Size:       " << payloadSize() << std::endl;
  std::cout << " Manual Timing:    " << manualTiming_ << std::endl;
  if (manualTiming_) {
    std::cout << " Sequence Side A:  " << getManualTiming(SideType::A) << std::endl;
    std::cout << " Sequence Side B:  " << getManualTiming(SideType::B) << std::endl;
  } else {
    std::cout << " Timing Side A:    " << getTiming(SideType::A) << std::endl;
    std::cout << " Timing Side B:    " << getTiming(SideType::B) << std::endl;
    std::cout << " Sequence Side A:  " << getArbTiming(SideType::A) << std::endl;
    std::cout << " Sequence Side B:  " << getArbTiming(SideType::B) << std::endl;
  }
  std::cout << " Oscillator:       " << getOscillator() << std::endl;
  std::cout << " Interlacing:      " << "[ " << getInterlacing(SideType::A) << ", " << getInterlacing(SideType::B) << " ]" << std::endl;
  std::cout << std::endl;
}

SensorType Sensor::type() const
{
  return stype_;
}

std::string Sensor::name() const
{
  return toString(stype_);
}

uint32_t Sensor::minframe() const
{
  return minframe_;
}

uint32_t Sensor::maxframe() const
{
  return maxframe_;
}

uint32_t Sensor::maxwidth() const
{
  return maxwidth_;
}

uint32_t Sensor::maxheight() const
{
  return maxheight_;
}

uint32_t Sensor::bytesperpixel() const
{
  return bytesperpixel_;
}

uint32_t Sensor::nframes() const
{
  return lastframe_ - firstframe_ + 1;
}

uint32_t Sensor::firstframe() const
{
  return firstframe_;
}

uint32_t Sensor::lastframe() const
{
  return lastframe_;
}

uint32_t Sensor::firstrow() const
{
  return firstrow_;
}

uint32_t Sensor::lastrow() const
{
  return lastrow_;
}

size_t Sensor::width() const
{
  return maxwidth_;
}

size_t Sensor::height() const
{
  return lastrow_ - firstrow_ + 1;
}

size_t Sensor::npixels() const
{
  return width() * height() * nframes();
}

size_t Sensor::payloadSize() const
{
  return npixels() * bytesperpixel_;
}

void Sensor::setRows()
{
  setRows(0, maxheight_ - 1);
}

void Sensor::setRows(uint32_t minrow, uint32_t maxrow)
{
  LOG_DEBUG << __func__ << " minrow " << minrow << " maxrow " << maxrow;
  if (minrow > maxrow || maxrow >= maxheight_) {
    LOG_EXCEPTION(InvalidROI, minrow, maxrow, "ROI row bounds are out of range");
  }

  board_->setRegister("FPA_ROW_INITIAL", minrow);
  board_->setRegister("FPA_ROW_FINAL", maxrow);

  firstrow_ = minrow;
  lastrow_ = maxrow;
}

void Sensor::setFrames()
{
  setFrames(minframe_, maxframe_);
}

void Sensor::setFrames(uint32_t minframe, uint32_t maxframe)
{
  LOG_DEBUG << __func__ << " minframe " << minframe << " maxframe " << maxframe;
  if (minframe < minframe_ || minframe > maxframe || maxframe > maxframe_) {
    LOG_EXCEPTION(InvalidROI, minframe, maxframe, "ROI frame bounds are out of range");
  }

  board_->setRegister("FPA_FRAME_INITIAL", minframe);
  board_->setRegister("FPA_FRAME_FINAL", maxframe);

  firstframe_ = minframe;
  lastframe_ = maxframe;
}

bool Sensor::manualTiming() const
{
  return manualTiming_;
}

void Sensor::initSensor()
{
  LOG_DEBUG << __func__;
  if (board_->type() == BoardType::LLNL_V1) {
    if (!checkSensorVoltStat()) {
      LOG_EXCEPTION(DeviceError, name(), "FPGA sensor select jumpers do not match with actual sensor");
    }
  }
  board_->initSensor();
  // initialize timing mode registers
  board_->setSubRegister("HST_MODE", manualTiming_ ? 0 : 1);
  board_->setSubRegister("MANSHUT_MODE", manualTiming_ ? 1 : 0);
}

Sequence Sensor::getArbTiming(SideType side) const
{
  LOG_DEBUG << __func__ << " side " << side;
  uint64_t timingreg = 0;
  Sequence sequence;
  if (side == SideType::A) {
    timingreg = Device::combineRegisters(board_->getRegister("HS_TIMING_DATA_ALO"), board_->getRegister("HS_TIMING_DATA_AHI")) & timing_bitmask;
  } else if (side == SideType::B) {
    timingreg = Device::combineRegisters(board_->getRegister("HS_TIMING_DATA_BLO"), board_->getRegister("HS_TIMING_DATA_BHI")) & timing_bitmask;
  } else {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid sensor side '" << side << "' for getTiming"));
  }

  uint64_t last = 0;
  uint32_t count = 0;
  bool isdelay = true;
  for (uint64_t bit=0; bit<=timing_bits; bit++) {
    uint32_t current = (timingreg >> bit) & 1;
    if (current != last) {
      if (isdelay) {
        count--;
        isdelay = false;
      }
      sequence.push_back(count);
      count = 1;
      last = current;
    } else {
      count++;
    }
  }

  return sequence;
}

Timing Sensor::getTiming(SideType side) const
{

  LOG_DEBUG << __func__ << " side " << side;
  Timing timing{0,0,0};
  Sequence timingSeq = getArbTiming(side);

  // if timingSeq is non-empty then calculate the timing
  if (!timingSeq.empty()) {
    timing.delay = timingSeq[0];
    timing.open = timingSeq[1];
    if (timingSeq.size() < 3) {
      timing.closed = timing_bits - (timingSeq[1] + timingSeq[0]);
    } else {
      timing.closed = timingSeq[2];
    }
  }

  // warn if there is extra effective delay from hidden pre-frames
  if (firstframe_ > 0) {
    uint32_t f0delay = timing.delay + firstframe_ * (timing.open + timing.closed);
    LOG_WARN << __func__ << " actual effective delay for " << type() << " sensor is " << f0delay;
  }

  return timing;
}

Sequence Sensor::getManualTiming(SideType side) const
{
  Sequence sequence(manual_seq_len);

  if (side == SideType::A) {
    sequence[0] = board_->getRegister("W0_INTEGRATION") * manual_seq_scale;
    sequence[1] = board_->getRegister("W0_INTERFRAME")  * manual_seq_scale;
    sequence[2] = board_->getRegister("W1_INTEGRATION") * manual_seq_scale;
    sequence[3] = board_->getRegister("W1_INTERFRAME")  * manual_seq_scale;
    sequence[4] = board_->getRegister("W2_INTEGRATION") * manual_seq_scale;
    sequence[5] = board_->getRegister("W2_INTERFRAME")  * manual_seq_scale;
    sequence[6] = board_->getRegister("W3_INTEGRATION") * manual_seq_scale;
  } else if (side == SideType::B) {
    sequence[0] = board_->getRegister("W0_INTEGRATION_B") * manual_seq_scale;
    sequence[1] = board_->getRegister("W0_INTERFRAME_B")  * manual_seq_scale;
    sequence[2] = board_->getRegister("W1_INTEGRATION_B") * manual_seq_scale;
    sequence[3] = board_->getRegister("W1_INTERFRAME_B")  * manual_seq_scale;
    sequence[4] = board_->getRegister("W2_INTEGRATION_B") * manual_seq_scale;
    sequence[5] = board_->getRegister("W2_INTERFRAME_B")  * manual_seq_scale;
    sequence[6] = board_->getRegister("W3_INTEGRATION_B") * manual_seq_scale;
  } else {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid sensor side '" << side << "' for getManualTiming"));
  }

  return sequence;
}

void Sensor::setArbTiming(SideType side, const Sequence& sequence)
{
  LOG_DEBUG << __func__ << " side " << side << " sequence " << toString(sequence);
  uint64_t timingreg = 0;
  uint64_t flag = 0;
  uint32_t count = 0;
  std::string lowreg;
  std::string highreg;
  bool isdelay = true;

  for (const auto seq : sequence) {
    uint64_t bits = std::min(isdelay ? seq + 1 : seq, timing_bits - count);
    if (flag) {
      timingreg |= ((flag<<bits) - 1) << count;
      flag = 0;
    } else {
      flag = 1;
    }
    count += (isdelay ? seq + 1 : seq);
    isdelay = false;
    if (count >= timing_bits) {
      break;
    }
  }

  bool wrote = false;
  uint32_t lowpart = timingreg & 0xffffffff;
  uint32_t highpart = timingreg >> 32;

  if (side == SideType::AB || side == SideType::A) {
    board_->setRegister("HS_TIMING_DATA_ALO", lowpart);
    board_->setRegister("HS_TIMING_DATA_AHI", highpart);
    if (manualTiming_) {
      timingCache_.clear();
      manualTiming_ = false;
    }
    timingCache_[SideType::A] = sequence;
    wrote = true;
  }
  if (side == SideType::AB || side == SideType::B) {
    board_->setRegister("HS_TIMING_DATA_BLO", lowpart);
    board_->setRegister("HS_TIMING_DATA_BHI", highpart);
    if (manualTiming_) {
      timingCache_.clear();
      manualTiming_ = false;
    }
    timingCache_[SideType::B] = sequence;
    wrote = true;
  }

  if (!wrote) {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid sensor side '" << side << "' for setArbTiming"));
  }

  board_->setSubRegister("MANSHUT_MODE", 0);
  board_->setSubRegister("HST_MODE", 1);

  Sequence actual;
  if (side == SideType::AB || side == SideType::A) {
    actual = getArbTiming(SideType::A);
  } else {
    actual = getArbTiming(SideType::B);
  }

  if (actual != sequence) {
    LOG_WARN << __func__ << " sequence truncated due to length, actual timing: " << actual;
  }
}

void Sensor::setTiming(SideType side, const Timing& timing)
{
  LOG_DEBUG << __func__ << " side " << side << " timing " << timing;
  uint32_t count = 1;
  Sequence sequence;

  // add the delay to the sequence
  sequence.push_back(timing.delay);
  count += timing.delay;

  if (std::all_of(interlacing_.begin(), interlacing_.end(), [](uint32_t x) { return x == 0; })) {
    // just fill 40bit register with needed number of sequences
    for (uint32_t idx=1; idx<nseq_; idx++) {
      sequence.push_back(timing.open);
      count += timing.open;
      sequence.push_back(timing.closed);
      count += timing.closed;
    }
    sequence.push_back(timing.open);
    count += timing.open;
  } else {
    // interlaced mode - try to fill the whole 40bit register as closely as we can
    while (count + timing.open <= timing_bits) {
      sequence.push_back(timing.open);
      count += timing.open;

      if (count + timing.closed + timing.open > timing_bits) {
        // another open won't fit so explicit close at end not needed
        break;
      }

      sequence.push_back(timing.closed);
      count += timing.closed;
    }
  }

  if (count > timing_bits) {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Timing sequence is too long to be implemented: " << count));
  }

  setArbTiming(side, sequence);

  // check the readback
  Sequence actual;
  if (side == SideType::AB || side == SideType::A) {
    actual = getArbTiming(SideType::A);
  } else {
    actual = getArbTiming(SideType::B);
  }

  // warn if there is extra effective delay from hidden pre-frames
  if (firstframe_ > 0) {
    uint32_t f0delay = timing.delay + firstframe_ * (timing.open + timing.closed);
    LOG_WARN << __func__ << " actual effective delay for " << type() << " sensor is " << f0delay;
  }

  if (actual != sequence) {
    LOG_WARN << __func__ << " sequence truncated due to length, actual timing: " << actual;
  }
}

void Sensor::setManualTiming(SideType side, const Sequence& sequence)
{
  LOG_DEBUG << __func__ << " side " << side << " sequence " << sequence;

  // check the length of the sequence
  if (sequence.size() != manual_seq_len) {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid manual shutter sequence length (actual vs. expected): " << sequence.size() << " vs. " << manual_seq_len));
  }

  // check the values are in valid range
  for (const auto& seq : sequence) {
    if (seq < min_manual_timing || seq > max_manual_timing) {
      LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid manual shutter timing: " << seq));
    }
  }

  bool wrote = false;

  if (side == SideType::AB || side == SideType::A) {
    board_->setRegister("W0_INTEGRATION", sequence[0]/manual_seq_scale);
    board_->setRegister("W0_INTERFRAME",  sequence[1]/manual_seq_scale);
    board_->setRegister("W1_INTEGRATION", sequence[2]/manual_seq_scale);
    board_->setRegister("W1_INTERFRAME",  sequence[3]/manual_seq_scale);
    board_->setRegister("W2_INTEGRATION", sequence[4]/manual_seq_scale);
    board_->setRegister("W2_INTERFRAME",  sequence[5]/manual_seq_scale);
    board_->setRegister("W3_INTEGRATION", sequence[6]/manual_seq_scale);
    if (!manualTiming_) {
      timingCache_.clear();
      manualTiming_ = true;
    }
    timingCache_[SideType::A] = sequence;
    wrote = true;
  }
  if (side == SideType::AB || side == SideType::B) {
    board_->setRegister("W0_INTEGRATION_B", sequence[0]/manual_seq_scale);
    board_->setRegister("W0_INTERFRAME_B",  sequence[1]/manual_seq_scale);
    board_->setRegister("W1_INTEGRATION_B", sequence[2]/manual_seq_scale);
    board_->setRegister("W1_INTERFRAME_B",  sequence[3]/manual_seq_scale);
    board_->setRegister("W2_INTEGRATION_B", sequence[4]/manual_seq_scale);
    board_->setRegister("W2_INTERFRAME_B",  sequence[5]/manual_seq_scale);
    board_->setRegister("W3_INTEGRATION_B", sequence[6]/manual_seq_scale);
    if (!manualTiming_) {
      timingCache_.clear();
      manualTiming_ = true;
    }
    timingCache_[SideType::B] = sequence;
    wrote = true;
  }

  if (!wrote) {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid sensor side '" << side << "' for setManualTiming"));
  }

  board_->setSubRegister("HST_MODE", 0);
  board_->setSubRegister("MANSHUT_MODE", 1);
}

void Sensor::setOscillator(OscillatorType osc)
{
  LOG_DEBUG << __func__;
  board_->setSubRegister("OSC_SELECT", static_cast<unsigned>(osc));
}

OscillatorType Sensor::getOscillator() const
{
  LOG_DEBUG << __func__;
  OscillatorType osc;
  uint32_t oscval = board_->getSubRegister("OSC_SELECT");
  switch (static_cast<OscillatorType>(oscval)) {
    case OscillatorType::RELAXATION:
    case OscillatorType::RING:
    case OscillatorType::RINGNOOSC:
    case OscillatorType::EXTERNAL:
      osc = static_cast<OscillatorType>(oscval);
      break;
    default:
      LOG_EXCEPTION(DeviceError, name(), LOG_STR("Invalid oscillator register value: " << oscval));
  }

  return osc;
}

void Sensor::setInterlacing(SideType side)
{
  LOG_WARN << __func__ << " Interlacing is not supported by " << type() << " sensors";
}

void Sensor::setInterlacing(uint32_t ifactor, SideType side)
{
  LOG_WARN << __func__ << " Interlacing is not supported by " << type() << " sensors";
}

uint32_t Sensor::getInterlacing(SideType side) const
{
  LOG_DEBUG << __func__;
  uint32_t ifactor = 0;
  if (side == SideType::A) {
    ifactor = interlacing_[0];
  } else if (side == SideType::B) {
    ifactor = interlacing_[1];
  } else {
    LOG_EXCEPTION(InvalidTiming, LOG_STR("Invalid sensor side '"  << side << "' for getInterlacing"));
  }

  return ifactor;
}

void Sensor::setHighFullWell(bool flag)
{
  LOG_WARN << __func__ << " HighFullWell mode is not supported by " << type() << " sensors";
}

bool Sensor::getHighFullWell() const
{
  return false;
}

void Sensor::setZeroDeadTime(bool flag, SideType side)
{
  LOG_WARN << __func__ << " ZeroDeadTime mode is not supported by " << type() << " sensors";
}

bool Sensor::getZeroDeadTime(SideType side) const
{
  return false;
}

void Sensor::setTriggerDelay()
{
  LOG_WARN << __func__ << " Trigger Delay is not supported by " << type() << " sensors";
}

void Sensor::setTriggerDelay(double delay)
{
  LOG_WARN << __func__ << " Trigger Delay is not supported by " << type() << " sensors";
}

double Sensor::getTriggerDelay() const
{
  return false;
}

void Sensor::setPhiDelay(SideType side)
{
  LOG_WARN << __func__ << " Phi Delay is not supported by " << type() << " sensors";
}

void Sensor::setPhiDelay(double delay, SideType side)
{
  LOG_WARN << __func__ << " Phi Delay is not supported by " << type() << " sensors";
}

double Sensor::getPhiDelay(SideType side)
{
  return 0.0;
}

void Sensor::setExtClk()
{
  LOG_WARN << __func__ << " External Phi Clock is not supported by " << type() << " sensors";
}

void Sensor::setExtClk(uint32_t dilation, double frequency)
{
  LOG_WARN << __func__ << " External Phi Clock is not supported by " << type() << " sensors";
}

std::unique_ptr<uint8_t[]> Sensor::readFrame8()
{
  LOG_DEBUG << __func__;
  return board_->getDataSubRegisterAsUInt8("READ_SRAM", 1, payloadSize());
}

std::unique_ptr<uint16_t[]> Sensor::readFrame16()
{
  LOG_DEBUG << __func__;
  if (bytesperpixel_ != 2) {
    LOG_EXCEPTION(DeviceError, name(), LOG_STR(__func__ << " requested 2 bytes per pixel does not match the sensor: " << bytesperpixel_));
  }
  return board_->getDataSubRegisterAsUInt16("READ_SRAM", 1, npixels());
}

std::unique_ptr<uint32_t[]> Sensor::readFrame32()
{
  LOG_DEBUG << __func__;
  if (bytesperpixel_ != 4) {
    LOG_EXCEPTION(DeviceError, name(), LOG_STR(__func__ << " requested 4 bytes per pixel does not match the sensor: " << bytesperpixel_));
  }
  return board_->getDataSubRegisterAsUInt32("READ_SRAM", 1, npixels());
}


void Sensor::restoreCachedTiming()
{
  for (const auto& kv : timingCache_) {
    if (manualTiming_) {
      setManualTiming(kv.first, kv.second);
    } else {
      setArbTiming(kv.first, kv.second);
    }
  }
}
