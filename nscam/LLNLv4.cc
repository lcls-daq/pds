#include "LLNLv4.hh"

static std::map<std::string, uint16_t> REG_NAMES = {
  {"FPGA_NUM", 0x000},
  {"FPGA_REV", 0x001},
  {"HS_TIMING_CTL", 0x010},
  {"HS_TIMING_DATA_ALO", 0x013},
  {"HS_TIMING_DATA_AHI", 0x014},
  {"HS_TIMING_DATA_BLO", 0x015},
  {"HS_TIMING_DATA_BHI", 0x016},
  {"SW_TRIGGER_CONTROL", 0x017},
  {"SW_COARSE_CONTROL", 0x01C},
  {"STAT_REG", 0x024},
  {"CTRL_REG", 0x025},
  {"DAC_CTL", 0x026},
  {"DAC_REG_A_AND_B", 0x027},
  {"DAC_REG_C_AND_D", 0x028},
  {"DAC_REG_E_AND_F", 0x029},
  {"DAC_REG_G_AND_H", 0x02A},
  {"SW_RESET", 0x02D},
  {"HST_SETTINGS", 0x02E},
  {"STAT_REG_SRC", 0x02F},
  {"STAT_REG2", 0x030},
  {"STAT_REG2_SRC", 0x031},
  {"ADC_BYTECOUNTER", 0x032},
  {"RBP_PIXEL_CNTR", 0x033},
  {"DIAG_MAX_CNT_0", 0x034},
  {"DIAG_MAX_CNT_1", 0x035},
  {"DIAG_CNTR_VAL_0", 0x036},
  {"DIAG_CNTR_VAL_1", 0x037},
  {"STAT_EDGE_DETECTS", 0x038},
  {"TRIGGER_CTL", 0x03A},
  {"SRAM_CTL", 0x03B},
  {"TIMER_CTL", 0x03C},
  {"TIMER_VALUE", 0x03D},
  {"HSTALLWEN_WAIT_TIME", 0x03F},
  {"FPA_ROW_INITIAL", 0x042},
  {"FPA_ROW_FINAL", 0x043},
  {"FPA_FRAME_INITIAL", 0x044},
  {"FPA_FRAME_FINAL", 0x045},
  {"FPA_DIVCLK_EN_ADDR", 0x046},
  {"FPA_OSCILLATOR_SEL_ADDR", 0x047},
  {"SUSPEND_TIME", 0x04D},
  {"FPA_INTERFACE_STATE", 0x04E},
  {"DELAY_READOFF", 0x04F},
  {"STAT_REG_SEC", 0x060},
  {"ADC_CTL", 0x090},
  {"ADC1_CONFIG_DATA", 0x091},
  {"ADC2_CONFIG_DATA", 0x092},
  {"ADC3_CONFIG_DATA", 0x093},
  {"ADC4_CONFIG_DATA", 0x094},
  {"ADC5_DATA_1", 0x095},
  {"ADC5_DATA_2", 0x096},
  {"ADC5_DATA_3", 0x097},
  {"ADC5_DATA_4", 0x098},
  {"ADC6_DATA_1", 0x099},
  {"ADC6_DATA_2", 0x09A},
  {"ADC6_DATA_3", 0x09B},
  {"ADC6_DATA_4", 0x09C},
  {"ADC_PPER", 0x09D},
  {"ADC_RESET", 0x09E},
};

static std::map<std::string, Pds::NsCam::SubRegister> SUBREGNAMES = {
  {"HST_MODE", {"HS_TIMING_CTL", 0, 1, true}},
  {"SW_TRIG_START", {"SW_TRIGGER_CONTROL", 0, 1, true}},
  {"SW_COARSE_TRIGGER", {"SW_COARSE_CONTROL", 0, 1, true}},
  {"LED_EN", {"CTRL_REG", 1, 1, true}},
  {"COLQUENCHEN", {"CTRL_REG", 2, 1, true}},
  {"POWERSAVE", {"CTRL_REG", 3, 1, true}},
  {"PDBIAS_LOW", {"CTRL_REG", 6, 1, true}},
  //{"SWACK", {"CTRL_REG", 10, 1, true}},
  {"DACA", {"DAC_REG_A_AND_B", 16, 16, true}},
  {"DACB", {"DAC_REG_A_AND_B", 0, 16, true}},
  {"DACC", {"DAC_REG_C_AND_D", 16, 16, true}},
  {"DACD", {"DAC_REG_C_AND_D", 0, 16, true}},
  {"DACE", {"DAC_REG_E_AND_F", 16, 16, true}},
  {"DACF", {"DAC_REG_E_AND_F", 0, 16, true}},
  {"DACG", {"DAC_REG_G_AND_H", 16, 16, true}},
  {"DACH", {"DAC_REG_G_AND_H", 0, 16, true}},
  {"RESET", {"SW_RESET", 0, 1, true}},
  {"HST_SW_CTL_EN", {"HST_SETTINGS", 0, 1, true}},
  {"SW_HSTALLWEN", {"HST_SETTINGS", 1, 1, true}},
  {"MAXERR_FIT", {"DIAG_MAX_CNT_0", 16, 16, true}},
  {"MAXERR_SRT", {"DIAG_MAX_CNT_0", 0, 8, true}},
  {"MAXERR_UTTR", {"DIAG_MAX_CNT_1", 16, 16, true}},
  {"MAXERR_URTR", {"DIAG_MAX_CNT_1", 0, 16, true}},
  {"HW_TRIG_EN", {"TRIGGER_CTL", 0, 1, true}},
  {"SW_TRIG_EN", {"TRIGGER_CTL", 2, 1, true}},
  {"READOFF_DELAY_EN", {"TRIGGER_CTL", 4, 1, true}},
  {"READ_SRAM", {"SRAM_CTL", 0, 1, true}},
  {"RESET_TIMER", {"TIMER_CTL", 0, 1, true}},
  {"OSC_SELECT", {"FPA_OSCILLATOR_SEL_ADDR", 0, 2, true}},
  {"PPER", {"ADC_PPER", 0, 8, true}},
  {"SRAM_READY", {"STAT_REG", 0, 1, false}},
  {"STAT_COARSE", {"STAT_REG", 1, 1, false}},
  {"STAT_FINE", {"STAT_REG", 2, 1, false}},
  {"STAT_SENSREADIP", {"STAT_REG", 5, 1, false}},
  {"STAT_SENSREADDONE", {"STAT_REG", 6, 1, false}},
  {"STAT_SRAMREADSTART", {"STAT_REG", 7, 1, false}},
  {"STAT_SRAMREADDONE", {"STAT_REG", 8, 1, false}},
  {"STAT_HSTCONFIGSTART", {"STAT_REG", 9, 1, false}},
  {"STAT_ADCSCONFIGURED", {"STAT_REG", 10, 1, false}},
  {"STAT_DACSCONFIGURED", {"STAT_REG", 11, 1, false}},
  {"STAT_TIMERCOUNTERRESET", {"STAT_REG", 13, 1, false}},
  {"STAT_HSTCONFIGDONE", {"STAT_REG", 16, 1, false}},
  {"STAT_ARMED", {"STAT_REG", 14, 1, false}},
  {"STAT_TEMP", {"STAT_REG", 17, 7, false}},
  {"STAT_PRESS", {"STAT_REG", 24, 8, false}},
  {"FPA_IF_TO", {"STAT_REG2", 0, 1, false}},
  {"SRAM_RO_TO", {"STAT_REG2", 1, 1, false}},
  {"PIXELRD_TOUT_ERR", {"STAT_REG2", 2, 1, false}},
  {"UART_TX_TO_RST", {"STAT_REG2", 3, 1, false}},
  {"UART_RX_TO_RST", {"STAT_REG2", 4, 1, false}},
  {"FIT_COUNT", {"DIAG_CNTR_VAL_0", 16, 16, false}},
  {"SRT_COUNT", {"DIAG_CNTR_VAL_0", 0, 8, false}},
  {"UTTR_COUNT", {"DIAG_CNTR_VAL_1", 16, 16, false}},
  {"URTR_COUNT", {"DIAG_CNTR_VAL_1", 0, 16, false}},
  {"MON_CH1", {"ADC5_DATA_1", 0, 12, false}},
  {"MON_CH2", {"ADC5_DATA_1", 12, 12, false}},
  {"MON_CH3", {"ADC5_DATA_2", 0, 12, false}},
  {"MON_CH4", {"ADC5_DATA_2", 12, 12, false}},
  {"MON_CH5", {"ADC5_DATA_3", 0, 12, false}},
  {"MON_CH6", {"ADC5_DATA_3", 12, 12, false}},
  {"MON_CH7", {"ADC5_DATA_4", 0, 12, false}},
  {"MON_CH8", {"ADC5_DATA_4", 12, 12, false}},
  {"MON_CH9", {"ADC6_DATA_1", 0, 12, false}},
  {"MON_CH10", {"ADC6_DATA_1", 12, 12, false}},
  {"MON_CH11", {"ADC6_DATA_2", 0, 12, false}},
  {"MON_CH12", {"ADC6_DATA_2", 12, 12, false}},
  {"MON_CH13", {"ADC6_DATA_3", 0, 12, false}},
  {"MON_CH14", {"ADC6_DATA_3", 12, 12, false}},
  {"MON_CH15", {"ADC6_DATA_4", 0, 12, false}},
  {"MON_CH16", {"ADC6_DATA_4", 12, 12, false}},
};

static std::map<std::string, uint32_t> DEFAULTS = {
};

static std::map<std::string, std::string> ICARUS_ALIASES = {
  {"HST_A_PDELAY", "DACA"},
  {"HST_A_NDELAY", "DACB"},
  {"HST_B_PDELAY", "DACC"},
  {"HST_B_NDELAY", "DACD"},
  {"HST_RO_IBIAS", "DACE"},
  {"HST_RO_NC_IBIAS", "DACE"},
  {"HST_OSC_CTL", "DACF"},
  {"VAB", "DACG"},
  {"VRST", "DACH"},
  {"MON_PRES_MINUS", "MON_CH1"},
  {"MON_PRES_PLUS", "MON_CH2"},
  {"MON_TEMP", "MON_CH3"},
  {"MON_COL_TOP_IBIAS_IN", "MON_CH4"},
  {"MON_HST_OSC_R_BIAS", "MON_CH5"},
  {"MON_VAB", "MON_CH6"},
  {"MON_HST_RO_IBIAS", "MON_CH7"},
  {"MON_HST_RO_NC_IBIAS", "MON_CH7"},
  {"MON_VRST", "MON_CH8"},
  {"MON_COL_BOT_IBIAS_IN", "MON_CH9"},
  {"MON_HST_A_PDELAY", "MON_CH10"},
  {"MON_HST_B_NDELAY", "MON_CH11"},
  {"DOSIMETER", "MON_CH12"},
  {"MON_HST_OSC_VREF_IN", "MON_CH13"},
  {"MON_HST_B_PDELAY", "MON_CH14"},
  {"MON_HST_OSC_CTL", "MON_CH15"},
  {"MON_HST_A_NDELAY", "MON_CH16"},
  {"MON_CHA", "MON_CH10"},
  {"MON_CHB", "MON_CH16"},
  {"MON_CHC", "MON_CH14"},
  {"MON_CHD", "MON_CH11"},
  {"MON_CHE", "MON_CH7"},
  {"MON_CHF", "MON_CH15"},
  {"MON_CHG", "MON_CH6"},
  {"MON_CHH", "MON_CH8"},
};

static std::map<std::string, std::string> DAEDALUS_ALIASES = {
  {"HST_OSC_VREF_IN", "DACC"},
  {"HST_OSC_CTL", "DACE"},
  {"COL_TST_IN", "DACF"},
  {"VAB", "DACG"},
  {"VRST", "DACH"},
  {"MON_PRES_MINUS", "MON_CH1"},
  {"MON_PRES_PLUS", "MON_CH2"},
  {"MON_TEMP", "MON_CH3"},
  {"MON_VAB", "MON_CH6"},
  {"MON_HST_OSC_CTL", "MON_CH7"},
  {"MON_TSENSE_OUT", "MON_CH10"},
  {"MON_BGREF", "MON_CH11"},
  {"DOSIMETER", "MON_CH12"},
  {"MON_HST_RO_NC_IBIAS", "MON_CH13"},
  {"MON_HST_OSC_VREF_IN", "MON_CH14"},
  {"MON_COL_TST_IN", "MON_CH15"},
  {"MON_HST_OSC_PBIAS_PAD", "MON_CH16"},
  {"MON_CHC", "MON_CH14"},
  {"MON_CHE", "MON_CH7"},
  {"MON_CHF", "MON_CH15"},
  {"MON_CHG", "MON_CH6"},
  {"MON_CHH", "MON_CH8"},
};

static std::map<std::string, std::string> ICARUS_MONITORS = {
  {"MON_CH10", "DACA"},
  {"MON_CH16", "DACB"},
  {"MON_CH14", "DACC"},
  {"MON_CH11", "DACD"},
  {"MON_CH7", "DACE"},
  {"MON_CH15", "DACF"},
  {"MON_CH6", "DACG"},
  {"MON_CH8", "DACH"},
};

static std::map<std::string, std::string> DAEDALUS_MONITORS = {
  {"MON_CH14", "DACC"},
  {"MON_CH7", "DACE"},
  {"MON_CH15", "DACF"},
  {"MON_CH6", "DACG"},
  {"MON_CH8", "DACH"},
};

using namespace Pds::NsCam;

LLNLv4::LLNLv4(SensorType stype, std::shared_ptr<Comm> comm) :
  Board(BoardType::LLNL_V4,
        stype,
        comm,
        REG_NAMES,
        SUBREGNAMES,
        DEFAULTS,
        stype == SensorType::DAEDALUS ? DAEDALUS_ALIASES : ICARUS_ALIASES,
        stype == SensorType::DAEDALUS ? DAEDALUS_MONITORS : ICARUS_MONITORS)
{}

void LLNLv4::initBoard()
{
  clearStatus();
  configADCs();

  // vref must be supplied externally for ADC128S102
  vref_ = 3.3;
  // set the adc multiplier
  adc5_mult_ = 1;
}

void LLNLv4::clearStatus()
{
  getRegister("STAT_REG_SRC");
  getRegister("STAT_REG2_SRC");
}

void LLNLv4::configADCs()
{
  // pull all adcs out of reset
  setRegister("ADC_RESET", 0x00000000);
  // workaround for uncertain behavior after previous readoff
  setRegister("ADC1_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC2_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC3_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC4_CONFIG_DATA", 0xFFFFFFFF);
  setRegister("ADC_CTL", 0xFFFFFFFF);
  setRegister("ADC1_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC2_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC3_CONFIG_DATA", 0x81A801FF);
  setRegister("ADC4_CONFIG_DATA", 0x81A801FF);
}

void LLNLv4::initPots()
{

}
