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


using namespace Pds::NsCam;

static std::string seq2str(const Sequence& sequence)
{
  bool first = true;
  std::stringstream seqstr;
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

static std::string timing2str(const Timing& timing)
{
  std::stringstream timingstr;
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
               uint32_t nframes) :
  stype_(stype),
  board_(board),
  nseq_(nframes),
  nframes_(nframes),
  firstframe_(0),
  ncols_(512),
  nrows_(1024),
  interlacing_{0, 0},
  manualTiming_(false)
{}

Sensor::Sensor(SensorType stype,
               std::shared_ptr<Board> board,
               uint32_t nseq,
               uint32_t nframes,
               uint32_t firstframe) :
  stype_(stype),
  board_(board),
  nseq_(nseq),
  nframes_(nframes),
  firstframe_(firstframe),
  ncols_(512),
  nrows_(1024),
  interlacing_{0, 0},
  manualTiming_(false)
{}

Sensor::Sensor(SensorType stype,
               std::shared_ptr<Board> board,
               uint32_t nseq,
               uint32_t nframes,
               uint32_t firstframe,
               uint32_t ncols,
               uint32_t nrows) :
  stype_(stype),
  board_(board),
  nseq_(nseq),
  nframes_(nframes),
  firstframe_(firstframe),
  ncols_(ncols),
  nrows_(nrows),
  interlacing_{0, 0},
  manualTiming_(false)
{}

void Sensor::info() const
{
  // save the i/o formatting before changing...
  std::ios_base::fmtflags fmt(std::cout.flags());
  std::cout << "Sensor Info:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Type:             " << name() << std::endl;
  std::cout << std::endl;
  // restore i/o formatting
  std::cout.flags(fmt);
}

SensorType Sensor::type() const
{
  return stype_;
}

std::string Sensor::name() const
{
  return toString(stype_);
}

uint32_t Sensor::nframes() const
{
  return nframes_;
}

bool Sensor::manualTiming() const
{
  return manualTiming_;
}

void Sensor::initSensor()
{
  if (board_->type() == BoardType::LLNL_V1) {
    if (!checkSensorVoltStat()) {
      LOG_EXCEPTION(BoardError(name(), "FPGA sensor select jumpers do not match with actual sensor"));
    }
  }
  board_->initSensor();
}

Sequence Sensor::getArbTiming(SideType side) const
{
  LOG_DEBUG(std::string(__func__) + " side " + toString(side));
  uint64_t timingreg = 0;
  Sequence sequence;
  if (side == SideType::A) {
    timingreg = Device::combineRegisters(board_->getRegister("HS_TIMING_DATA_ALO"), board_->getRegister("HS_TIMING_DATA_AHI")) & timing_bitmask;
  } else if (side == SideType::B) {
    timingreg = Device::combineRegisters(board_->getRegister("HS_TIMING_DATA_BLO"), board_->getRegister("HS_TIMING_DATA_BHI")) & timing_bitmask;
  } else {
    LOG_EXCEPTION(InvalidTiming("Invalid sensor side '" + std::string(toString(side)) + "' for getTiming"));
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

  LOG_DEBUG(std::string(__func__) + " side " + toString(side));
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
    LOG_WARN(std::string(__func__) + " actual effective delay for " + name() + " sensor is " + std::to_string(f0delay));
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
    LOG_EXCEPTION(InvalidTiming("Invalid sensor side '" + std::string(toString(side)) + "' for getManualTiming"));
  }

  return sequence;
}

void Sensor::setArbTiming(SideType side, const Sequence& sequence)
{
  LOG_DEBUG(std::string(__func__) + " side " + toString(side) + " sequence " + seq2str(sequence));
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
    LOG_EXCEPTION(InvalidTiming("Invalid sensor side '" + std::string(toString(side)) + "' for setArbTiming"));
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
    LOG_WARN(std::string(__func__) + " sequence truncated due to length, actual timing: " + seq2str(actual));
  }
}

void Sensor::setTiming(SideType side, const Timing& timing)
{
  LOG_DEBUG(std::string(__func__) + " side " + toString(side) + " timing " + timing2str(timing));
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
    LOG_EXCEPTION(InvalidTiming("Timing sequence is too long to be implemented: " + std::to_string(count)));
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
    LOG_WARN(std::string(__func__) + " actual effective delay for " + name() + " sensor is " + std::to_string(f0delay));
  }

  if (actual != sequence) {
    LOG_WARN(std::string(__func__) + " sequence truncated due to length, actual timing: " + seq2str(actual));
  }
}

void Sensor::setManualTiming(SideType side, const Sequence& sequence)
{
  LOG_DEBUG(std::string(__func__) + " side " + toString(side) + " sequence " + seq2str(sequence));

  // check the length of the sequence
  if (sequence.size() != manual_seq_len) {
    LOG_EXCEPTION(InvalidTiming("Invalid manual shutter sequence length (actual vs. expected): " + std::to_string(sequence.size()) + " vs. " + std::to_string(manual_seq_len)));
  }

  // check the values are in valid range
  for (const auto& seq : sequence) {
    if (seq < min_manual_timing || seq > max_manual_timing) {
      LOG_EXCEPTION(InvalidTiming("Invalid manual shutter timing: " + std::to_string(seq)));
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
    LOG_EXCEPTION(InvalidTiming("Invalid sensor side '" + std::string(toString(side)) + "' for setManualTiming"));
  }

  board_->setSubRegister("HST_MODE", 0);
  board_->setSubRegister("MANSHUT_MODE", 1);
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
