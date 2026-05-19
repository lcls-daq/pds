#include "Icarus2.hh"
#include "Error.hh"

static std::map<std::string, uint16_t> REG_NAMES = {
  {"VRESET_WAIT_TIME", 0x03E},
  {"ICARUS_VER_SEL", 0x041},
  {"MISC_SENSOR_CTL", 0x04C},
  {"MANUAL_SHUTTERS_MODE", 0x050},
  {"W0_INTEGRATION", 0x051},
  {"W0_INTERFRAME", 0x052},
  {"W1_INTEGRATION", 0x053},
  {"W1_INTERFRAME", 0x054},
  {"W2_INTEGRATION", 0x055},
  {"W2_INTERFRAME", 0x056},
  {"W3_INTEGRATION", 0x057},
  {"W0_INTEGRATION_B", 0x058},
  {"W0_INTERFRAME_B", 0x059},
  {"W1_INTEGRATION_B", 0x05A},
  {"W1_INTERFRAME_B", 0x05B},
  {"W2_INTEGRATION_B", 0x05C},
  {"W2_INTERFRAME_B", 0x05D},
  {"W3_INTEGRATION_B", 0x05E},
  {"TIME_ROW_DCD", 0x05F},
};

static std::map<std::string, Pds::NsCam::SubRegister> SUBREGNAMES = {
  {"MANSHUT_MODE", {"MANUAL_SHUTTERS_MODE", 0, 1, true}},
  {"REVREAD", {"CTRL_REG", 4, 1, true}},
  {"PDBIAS_LOW", {"CTRL_REG", 6, 1, true}},
  {"ROWDCD_CTL", {"CTRL_REG", 7, 1, true}},
  {"ACCUMULATION_CTL", {"MISC_SENSOR_CTL", 0, 1, true}},
  {"HST_TST_ANRST_EN", {"MISC_SENSOR_CTL", 1, 1, true}},
  {"HST_TST_BNRST_EN", {"MISC_SENSOR_CTL", 2, 1, true}},
  {"HST_TST_ANRST_IN", {"MISC_SENSOR_CTL", 3, 1, true}},
  {"HST_TST_BNRST_IN", {"MISC_SENSOR_CTL", 4, 1, true}},
  {"HST_PXL_RST_EN", {"MISC_SENSOR_CTL", 5, 1, true}},
  {"HST_CONT_MODE", {"MISC_SENSOR_CTL", 6, 1, true}},
  {"COL_DCD_EN", {"MISC_SENSOR_CTL", 7, 1, true}},
  {"COL_READOUT_EN", {"MISC_SENSOR_CTL", 8, 1, true}},
  {"STAT_W3TOPAEDGE1", {"STAT_REG", 3, 1, false}},
  {"STAT_W3TOPBEDGE1", {"STAT_REG", 4, 1, false}},
  {"STAT_HST_ALL_W_EN_DETECTED", {"STAT_REG", 12, 1, false}},
  {"PDBIAS_UNREADY", {"STAT_REG2", 5, 1, false}},
};

static std::map<std::string, uint32_t> DEFAULTS = {
  {"ICARUS_VER_SEL", 0x00000000},
  {"FPA_FRAME_INITIAL", 0x00000000},
  {"FPA_FRAME_FINAL", 0x00000003},
  {"FPA_ROW_INITIAL", 0x00000000},
  {"FPA_ROW_FINAL", 0x000003FF},
  {"HS_TIMING_DATA_BHI", 0x00000000},
  {"HS_TIMING_DATA_BLO", 0x00006666},
  {"HS_TIMING_DATA_AHI", 0x00000000},
  {"HS_TIMING_DATA_ALO", 0x00006666},
};

static std::map<std::string, uint16_t> LLNLV1_REGNAMES = {
};

static std::map<std::string, uint16_t> LLNLV4_REGNAMES = {
  {"DELAY_ASSERTION_ROWDCD_EN", 0x04F},
};

static std::map<std::string, Pds::NsCam::SubRegister> LLNLV1_SUBREGNAMES = {
};

static std::map<std::string, Pds::NsCam::SubRegister> LLNLV4_SUBREGNAMES = {
};

static std::map<std::string, uint32_t> LLNLV1_DEFAULTS = {
};

static std::map<std::string, uint32_t> LLNLV4_DEFAULTS = {
};

using namespace Pds::NsCam;

Icarus2::Icarus2(BoardType btype, std::shared_ptr<Comm> comm) :
  Sensor(SensorType::ICARUS, btype, comm, REG_NAMES, SUBREGNAMES, DEFAULTS)
{
  // Add board specific sensor register addresses
  const std::map<std::string, uint16_t>& boardregs = btype == BoardType::LLNL_V1 ? LLNLV1_REGNAMES : LLNLV4_REGNAMES;
  for (const auto& kv : boardregs) {
    addRegister(kv.first, kv.second);
  }

  // Add board specific sensor subregister addresses
  const std::map<std::string, Pds::NsCam::SubRegister>& boardsubregs = btype == BoardType::LLNL_V1 ? LLNLV1_SUBREGNAMES : LLNLV4_SUBREGNAMES;
  for (const auto& kv : boardsubregs) {
    addSubRegister(kv.first, kv.second);
  }

  // Add board specific sensor subregister addresses
  const std::map<std::string, uint32_t>& boarddefs = btype == BoardType::LLNL_V1 ? LLNLV1_DEFAULTS : LLNLV4_DEFAULTS;
  for (const auto& kv : boarddefs) {
    addDefault(kv.first, kv.second);
  }
}
